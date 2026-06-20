#include "n_queens_sim.h"
#include <omp.h>
#include <cmath>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

NQueensSimulator::NQueensSimulator() {
    boardSizeN = 8;       // Default N
    numThreads = 4;       // Default Threads
    executionDelayMs = 50; // Speed of the visualization
    
    isRunning = false;
    isStopped = true;
    totalSolutions = 0;
    camZoom = 1.0;
    
    threadStates.resize(16); // Support up to 16 threads visually
}

NQueensSimulator::~NQueensSimulator() {
    isStopped = true;
    isRunning = false;
    if (solverThread.joinable()) solverThread.join();
}

// --- OPENMP SOLVER LOGIC ---

void NQueensSimulator::placeQueen(int* queens, int row, int col, int threadId) {
    if (isStopped) return;
    
    // Pause loop
    while (!isRunning && !isStopped) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Update the visual state for this thread
    threadStates[threadId].active = true;
    for (int i = 0; i < row; i++) threadStates[threadId].currentQueens[i] = queens[i];
    threadStates[threadId].currentQueens[row] = col;
    threadStates[threadId].currentRow = row;

    // Artificial delay to make parallel visualization visible
    std::this_thread::sleep_for(std::chrono::milliseconds(executionDelayMs));

    // Standard N-Queens validation
    for(int i = 0; i < row; i++) {
        if (queens[i] == col) return; 
        if (std::abs(queens[i] - col) == (row - i)) return; 
    }
    
    queens[row] = col;
    
    if (row == boardSizeN - 1) {
        #pragma omp atomic
        totalSolutions++;
        threadStates[threadId].solutionsFound++;
    } else {
        for(int i = 0; i < boardSizeN; i++) {
            placeQueen(queens, row + 1, i, threadId);
        }
    }
    
    // Clear state on backtrack
    threadStates[threadId].currentRow = row - 1;
}

void NQueensSimulator::solveParallel() {
    omp_set_num_threads(numThreads);
    
    #pragma omp parallel 
    #pragma omp single
    {
        for(int i = 0; i < boardSizeN; i++) {
            if (isStopped) break;
            
            #pragma omp task
            { 
                int tid = omp_get_thread_num();
                int* localQueens = new int[boardSizeN];
                placeQueen(localQueens, 0, i, tid);
                delete[] localQueens;
                threadStates[tid].active = false; // Task finished
            }
        }
    }
    isRunning = false;
    isStopped = true;
}

void NQueensSimulator::startSolver() {
    isStopped = true;
    if (solverThread.joinable()) solverThread.join();
    
    // Reset States
    for (auto& ts : threadStates) {
        ts.active = false;
        ts.currentRow = -1;
        ts.solutionsFound = 0;
        ts.lastSolutionsFound = 0; 
        ts.flashTimer = 0.0f;
    }
    totalSolutions = 0;
    
    isStopped = false;
    isRunning = true;
    solverThread = std::thread(&NQueensSimulator::solveParallel, this);
}

// --- RENDER LOGIC ---

void NQueensSimulator::drawSingleBoard(float startX, float startY, float size, int threadId, double deltaTime) {
    float cellSize = size / boardSizeN;
    ThreadState& ts = threadStates[threadId];
    
    // --- NEW: FLASH LOGIC ---
    if (ts.solutionsFound > ts.lastSolutionsFound) {
        ts.flashTimer = 0.5f; // Flash lasts for 0.5 seconds
        ts.lastSolutionsFound = ts.solutionsFound;
    }
    
    if (ts.flashTimer > 0.0f) {
        ts.flashTimer -= deltaTime;
        if (ts.flashTimer < 0.0f) ts.flashTimer = 0.0f;
    }
    
    // Calculate intensity (0.0 to 1.0)
    float flashIntensity = ts.flashTimer / 0.5f;
    // Draw Board Grid
    glBegin(GL_QUADS);
    for (int r = 0; r < boardSizeN; r++) {
        for (int c = 0; c < boardSizeN; c++) {
            // Base colors
            float baseR = ((r + c) % 2 == 0) ? 0.8f : 0.3f;
            float baseG = ((r + c) % 2 == 0) ? 0.8f : 0.4f;
            float baseB = ((r + c) % 2 == 0) ? 0.8f : 0.5f;
            
            // Blend with Bright Green if flashing
            float finalR = baseR * (1.0f - flashIntensity) + 0.0f * flashIntensity;
            float finalG = baseG * (1.0f - flashIntensity) + 1.0f * flashIntensity;
            float finalB = baseB * (1.0f - flashIntensity) + 0.0f * flashIntensity;

            glColor3f(finalR, finalG, finalB);
            
            float cx = startX + c * cellSize;
            float cy = startY + r * cellSize;
            
            glVertex2f(cx, cy);
            glVertex2f(cx + cellSize, cy);
            glVertex2f(cx + cellSize, cy + cellSize);
            glVertex2f(cx, cy + cellSize);
        }
    }
    glEnd();

    // Draw Queens
    if (ts.active) {
        glColor3f(1.0f, 0.2f, 0.2f); // Red Queen
        glPointSize(cellSize * 0.6f);
        glBegin(GL_POINTS);
        for (int r = 0; r <= ts.currentRow; r++) {
            int c = ts.currentQueens[r];
            float cx = startX + c * cellSize + (cellSize / 2.0f);
            float cy = startY + r * cellSize + (cellSize / 2.0f);
            glVertex2f(cx, cy);
        }
        glEnd();
    }
}

void NQueensSimulator::drawChessboards(int winWidth, int winHeight, double deltaTime) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, winWidth, winHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Arrange boards in a grid based on number of threads
    int cols = std::ceil(std::sqrt(numThreads));
    int rows = std::ceil((float)numThreads / cols);
    
    float padding = 20.0f;
    float availWidth = (winWidth - 320) - (padding * (cols + 1)); // 320 is sidebar width
    float availHeight = winHeight - (padding * (rows + 1));
    
    float boardSize = std::min(availWidth / cols, availHeight / rows) * camZoom;
    
    float offsetX = 320 + padding;
    float offsetY = padding;

    for (int i = 0; i < numThreads; i++) {
        int r = i / cols;
        int c = i % cols;
        float x = offsetX + c * (boardSize + padding);
        float y = offsetY + r * (boardSize + padding);
        
        drawSingleBoard(x, y, boardSize, i, deltaTime);
        
        // Thread Label via ImGui background draw list
        std::string label = "Thread " + std::to_string(i) + "\nSol: " + std::to_string(threadStates[i].solutionsFound);
        if (!threadStates[i].active && !isStopped) label += " (Waiting)";
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y - 18), IM_COL32(0, 255, 100, 255), label.c_str());
    }
}

// --- UI LOGIC ---

void NQueensSimulator::renderUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(320, ImGui::GetIO().DisplaySize.y));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
                                    
    ImGui::Begin("Sidebar", nullptr, flags);

    ImGui::Text("OPENMP SHOWCASE");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped("1. THE N-QUEENS PROBLEM\nThe puzzle requires placing N chess queens on an NxN board so that no two queens threaten each other.");
    ImGui::Spacing();
    
    ImGui::TextWrapped("2. BACKTRACKING\nThe algorithm places queens row by row. If a placement leads to a conflict, it backtracks to the previous row.");
    ImGui::Spacing();
    
    ImGui::TextWrapped("3. OPENMP TASKS\nThe master thread loops through the first row and generates an OpenMP '#pragma omp task' for each column. Worker threads dynamically pick up these tasks and solve the branches in parallel.");
    ImGui::Spacing();

    ImGui::TextWrapped("4. HARDWARE CONCERNS\nYou can tell the UI to spawn 16 threads, and OpenMP will gladly do it. However, because we only have 2 physical cores(Intel Core 2 Duo P8600), the operating system will have to constantly pause and swap those 16 threads in and out of the 2 available slots (called context switching). This overhead actually makes the simulation slower than just running 2 threads.");
    ImGui::Spacing();
    ImGui::Separator();
    
    ImGui::Text("PARAMETERS");
    ImGui::BeginDisabled(!isStopped); // Disable changing N and Threads while running
    ImGui::SliderInt("Board Size (N)", &boardSizeN, 4, 16);
    ImGui::SliderInt("Threads", &numThreads, 1, 16);
    ImGui::EndDisabled();
    
    ImGui::SliderInt("Delay (ms)", &executionDelayMs, 0, 500);
    
    ImGui::Spacing();
    ImGui::Separator();
    
    ImGui::Text("STATUS: %s", isStopped ? "STOPPED" : (isRunning ? "RUNNING" : "PAUSED"));
    ImGui::Text("TOTAL SOLUTIONS: %d", totalSolutions);
    
    ImGui::Spacing();
    
    if (ImGui::Button(isRunning ? "PAUSE" : (isStopped ? "START" : "RESUME"), ImVec2(140, 40))) {
        if (isStopped) startSolver();
        else isRunning = !isRunning;
    }
    ImGui::SameLine();
    if (ImGui::Button("STOP", ImVec2(140, 40))) {
        isStopped = true;
        isRunning = false;
    }

    // --- ZOOM BUTTONS ---
    ImGui::Text("VIEW CONTROLS");
    if (ImGui::Button("Zoom In [+]", ImVec2(140, 30))) {
        camZoom += 0.1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Zoom Out [-]", ImVec2(140, 30))) {
        camZoom -= 0.1;
        if (camZoom < 0.1) camZoom = 0.1; // Prevent zooming into negative space
    }
    ImGui::Spacing();
    ImGui::Separator();
    // -------------------------

    ImGui::End();
}

void NQueensSimulator::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    NQueensSimulator* sim = (NQueensSimulator*)glfwGetWindowUserPointer(window);
    sim->camZoom += yoffset * 0.05;
    if (sim->camZoom < 0.1) sim->camZoom = 0.1;
}

void NQueensSimulator::run() {
    if (!glfwInit()) return;
    GLFWwindow* window = glfwCreateWindow(1000, 600, "OpenMP N-Queens", NULL, NULL);
    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, scrollCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130"); 

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderUI();

        int winWidth, winHeight;
        glfwGetFramebufferSize(window, &winWidth, &winHeight);
        glViewport(0, 0, winWidth, winHeight);
        glClearColor(0.08f, 0.1f, 0.12f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the visualization mapping
        drawChessboards(winWidth, winHeight, deltaTime);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
    
    isStopped = true;
    isRunning = false;
    if (solverThread.joinable()) solverThread.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
}