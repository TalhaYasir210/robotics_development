# CRITICAL CONSTRAINT: Environment Specific Parameter Management

**WARNING TO AGENT:** DO NOT EVER change a configuration value (in `explore.yaml`, `nav2_params_slam.yaml`, or any other config) without preserving the previous environment's value! Failure to do so breaks the modularity of the project.

When making parameter changes to fix issues for a specific map or environment:
1. **Never overwrite or delete** previously working parameters for other environments.
2. Instead, comment out the old parameters, add the new ones, and clearly label them with the target environment (e.g., `[OFFICE]` vs `[WAREHOUSE]`). 
   *Example Format:*
   ```yaml
      # --- ENVIRONMENT SPECIFIC PARAMS ---
      # [OFFICE] Use 0.25 for X
      # param_name: 0.25
      
      # [WAREHOUSE] Use 0.10 for Y
      param_name: 0.10
      # -----------------------------------
   ```
3. **Always update the `README.md`** immediately in the same step to describe what these parameters do, why they were changed, and explicitly instruct the user on how to toggle them when switching between different maps.
# Autonomous Navigation - Project Context & State

**Note to Gemini Agent (or self):** If you are reading this in a new session, this file contains the architectural decisions and current progress for the `autonomous_navigation` project. 
**Crucial Constraint:** The user is doing this for learning. ALWAYS explain the "why" before the "how". Proceed strictly step-by-step. NEVER skip writing detailed GTests for ANY `.cpp` file.

## Architectural Decisions
- **Robot:** Clearpath Jackal or TurtleBot3 (differential drive + 2D Lidar).
- **GUI:** PyQt5 (modular node).
- **Auto SLAM:** Frontier exploration integration.
- **Pause/Resume:** Uses Nav2 cancel and replace trajectory mechanism.
- **Logging:** All logs are output to a dedicated `logs/` directory in the workspace.
- **Testing:** Comprehensive, highly granular GTest suites for all C++ logic.
- **Workflow Automation:** The AI agent MUST automatically run `colcon build --packages-select <package>` and `colcon test --packages-select <package>` on its own. **NEVER** run a full `colcon build` without specifying packages. It should only ask the user to verify with `colcon test-result` before committing.
- **Self-Contained Assets (No Hardcoded Paths):** All maps, worlds, and models must be placed directly inside the project (`autonomous_navigation/worlds`, `maps`, `models`, etc.). Paths in launch files or C++ code must NEVER be hardcoded (use `get_package_share_directory()` or similar) so the project works instantly out-of-the-box for anyone cloning the repo.
  - *Note on Standalone:* "Standalone" does NOT mean copying standard external libraries or prerequisites (like default Nav2 yaml files, standard ROS packages, etc.) into the workspace. Rely on standard system-installed ROS 2 packages.
  - *No External Modifications:* Do NOT change any external files outside the workspace just to make the project run. All necessary custom configurations, overrides, and launch files must be contained entirely within this workspace. This ensures the project runs smoothly on any user's PC without altering their core system setup.
- **Version Control (Git):** Every completed feature (e.g., custom message, core logic, gtest) MUST be stored as an individual Git commit with a clear and descriptive message. The AI agent will explicitly instruct the user when it is time to commit at the end of a successful step.

## Progress / Workflow Tracking
- [x] Initial Requirements Gathering & Planning.
- [x] **Step 1:** Define Custom ROS 2 Messages & Design Nav2 Core Logic.
- [x] **Step 2:** Write GTests for Nav2 Core Logic.
- [x] **Step 3:** Implement C++ Nav2 Action Client.
- [x] **Step 4:** Gazebo Environments (Office & Warehouse).
- [x] **Step 5:** Auto SLAM Mapping Setup.
- [ ] **Step 6:** PyQt5 GUI Implementation.
  - **Development Strategy:**
    - **Current Focus:** Focus entirely on front-end UI/UX design, layout creation, and progressive interface flow. (Do not write backend functionality yet).when layout is finalized then move to backend integration.
    - **Flexibility Note:** This front-end workflow and feature set are dynamic; the UI flow can be modified or expanded over time as new requirements arise.
  - **Global UI Requirements & Aesthetic ("Swiss" Design):**
    - **Persistent Logging Screen:** A light gray (`#E4E7EA`) panel must be continuously visible and anchored at the bottom (20-25%) of the screen throughout every step of the flow. This acts as a terminal with dark monospace text.
    - **Strictly No Extraneous Icons:** The interface must remain entirely focused on the task. Do not include any OS-style status indicators or generic icons.
    - **Main Canvas:** Pure White (`#ffffff`) with a subtle drop shadow over an off-white/light-gray background (`#F0F2F4`). Dark Charcoal (`#1A1D21`) typography.
    - **Universal Back Button:** A white button with a light gray border placed in the top left corner (on pages 2-5).
  - **GUI Interaction Flow (5-Page Progressive Disclosure):**
    - The GUI follows a strict 5-page transition flow using a QStackedWidget.
    - **Page 1: Initialization State:** 
      - Heading: "Map Selection". Centered question: "On which map you want to navigate the Bot".
      - Component: Single dropdown menu "Select Map" (White bg, light border, Vibrant Teal chevron).
    - **Page 2: Mode Selection State:**
      - Heading: Dynamic Map Name (e.g. "Map: Factory_Floor_1"). Centered question: "Do you want to do navigation or Mapping".
      - Components: Two side-by-side buttons. Left: "Autonomous Navigation" (Vibrant Teal `#0D9488`), Right: "SLAM Mapping" (Muted Sage Green `#6B8F71`).
    - **Page 3: Navigation Action State:**
      - Heading: "Autonomous Navigation Mode". Prompt: "Give destination pose via RViz".
      - Components: Split view (Left: RViz map, Right: Gazebo camera).
      - Controls (below feeds): "Pause Navigation" (Cornflower Blue `#6B8FD4`) and "Cancel Navigation" (Coral/Red `#E57373`).
    - **Page 4: Auto Mapping State:**
      - Heading: "Auto SLAM Mapping".
      - Components: Split view (Left: RViz map building, Right: Gazebo camera). No manual controls here.
    - **Page 5: Manual Mapping State:**
      - Heading: "Manual SLAM Mapping".
      - Components: Split view (Left: RViz map, Right: Gazebo camera).
      - Controls: Minimalist keyboard guide overlay formatted like a D-pad (I, J, K, L) placed below the feeds.
- [ ] **Step 7:** Master Launch & Polish.
