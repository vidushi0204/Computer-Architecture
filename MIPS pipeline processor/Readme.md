# COL216 Assignment 2 - Pipeline Simulation and Branch Prediction

## Overview:
This project focuses on simulating different stages of instruction pipelines (5-stage and 7-9 stage) with and without bypassing. It also explores the performance impact of branch prediction strategies, including a 2-bit counter and a combination of branch history registers (BHR) and counters.

## Key Components:
1. **Pipeline Simulation**:
   - Simulates the execution of instructions in a 5-stage pipeline and a 7-9 stage pipeline, with and without bypassing.
   - Handles data dependencies between instructions by placing locks on registers to avoid overwriting values prematurely.
   - Bypassing reduces stalls in the pipeline by forwarding data directly from the execution stages instead of waiting for the write-back stage.

2. **Branch Prediction**:
   - **2-bit Saturating Counter**: Uses a 2-bit counter to predict branches based on the 14 least significant bits of the program counter (PC).
   - **Branch History Register (BHR)**: Maintains a history of branches and predicts based on past patterns.
   - **BHR + Counter**: Combines the BHR and a 2-bit counter for a more sophisticated branch prediction approach.

## Code Structure:
### 1. Pipeline Simulation:
   - The simulation iterates through instructions, stalling the pipeline when necessary due to data hazards or control hazards (e.g., branching).
   - For bypassing pipelines, locks on registers are released earlier when the result is available in the execution stage, improving performance.

### 2. Branch Prediction:
   - Implements three prediction strategies and calculates accuracy based on different initial predictor states (`00`, `01`, `10`, `11`).
   - Accuracy is computed for each strategy across a given input file, measuring how well each predictor handles branches.

## Results:
### 1. Pipeline Performance:
   - **5-stage Pipeline (without bypassing)**: 89 cycles.
   - **5-stage Pipeline (with bypassing)**: 88 cycles.
   - **7-9 stage Pipeline (without bypassing)**: 141 cycles.
   - **7-9 stage Pipeline (with bypassing)**: 140 cycles.
   - Bypassing reduces the number of stalls and improves performance.

### 2. Branch Prediction Accuracy:
   - The accuracy of different branch prediction strategies is evaluated for various initial states, with the **2-bit Counter** strategy showing the highest overall accuracy (up to 87.95%).

## Observations:
- A **7-9 stage pipeline** takes more clock cycles than a **5-stage pipeline** due to the increased number of stages.
- Pipelines **with bypassing** outperform those without bypassing by reducing stalls.
- Branch prediction accuracy varies depending on the strategy, with the **BHR + Counter** combination improving prediction accuracy over standalone approaches.
