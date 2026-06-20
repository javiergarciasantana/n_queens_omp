#ifndef NQUEENS_SIM_H
#define NQUEENS_SIM_H

#include <vector>
#include <atomic>
#include <thread>
#include <GLFW/glfw3.h>

// Tracks the real-time state of a specific OpenMP thread
struct ThreadState {
    bool active = false;
    int currentQueens[32]; // up to N=32
    int currentRow = -1;
    int solutionsFound = 0;
    int lastSolutionsFound = 0; 
    float flashTimer = 0.0f;
};

class NQueensSimulator {
public:
    NQueensSimulator();
    ~NQueensSimulator();
    void run();

private:
    // User Controls
    int boardSizeN;
    int numThreads;
    int executionDelayMs;
    
    // Simulation State
    std::atomic<bool> isRunning;
    std::atomic<bool> isStopped;
    //std::atomic<int> totalSolutions;
    // Here, we do not sue std::atomic<int> bc OPM will try to apply an atomic
    // wrapper for the var, and it will get confused.
    // std::atomic<std::atomic<int> > --> std::atomic<int>
    int totalSolutions;
    
    // Thread tracking
    std::vector<ThreadState> threadStates;
    std::thread solverThread;

    // OpenMP Logic
    void startSolver();
    void solveParallel();
    void placeQueen(int* queens, int row, int col, int threadId);

    // Rendering
    void renderUI();
    void drawChessboards(int winWidth, int winHeight, double deltaTime);
    void drawSingleBoard(float x, float y, float size, int threadId, double deltaTime);

    // GLFW
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    double camZoom;
};

#endif