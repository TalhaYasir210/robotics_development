# Comprehensive AI Prompt for Project Recreation

Copy the prompt below and paste it into any advanced AI agent (like Gemini 1.5 Pro / Antigravity) to perfectly recreate the entire `autonomous_navigation` project from scratch. This prompt has been meticulously crafted to include every architectural decision, strict constraint, and design nuance so the AI doesn't miss a single detail.

---

**Prompt for the AI Agent:**

You are an expert Robotics Software Engineer and AI Assistant. Your task is to build a highly modular, standalone, and rigorously tested ROS 2 (Jazzy) workspace for autonomous navigation and mapping. The robot used will be a differential drive robot (e.g., TurtleBot3 or Jackal). 

You MUST adhere to the following architectural constraints and instructions. Do not skip any steps. Proceed strictly step-by-step, asking for my confirmation after each step is fully implemented and tested.

### 1. Architectural Rules & Constraints
- **Standalone Workspace:** The project must be fully self-contained. All maps, worlds, and models must be placed directly inside the project (`worlds/`, `maps/`, `models/`). 
- **No Hardcoded Paths:** In C++ code and Python launch files, NEVER use hardcoded absolute paths. Always use `ament_index_cpp::get_package_share_directory` or Python's `get_package_share_directory`.
- **Environment-Specific Parameters (CRITICAL RULE):** Do not ever change a configuration value (in `explore.yaml`, `mapper_params_online_async.yaml`, or `nav2_params_slam.yaml`) without preserving the previous environment's value. If tuning for a new map (e.g., Office vs. Warehouse), comment out the old parameters, add the new ones, and clearly label them (e.g., `# [OFFICE] Use 0.25`, `[WAREHOUSE] param_name: 0.10`). Document this toggle process in the README.
- **Logging:** All ROS logs must output to a dedicated `logs/` directory at the workspace root.
- **Workflow Automation:** You must automatically run `colcon build --packages-select <package>` and `colcon test --packages-select <package>` when building. Never run a full `colcon build` without specifying packages.
- **Git Versioning:** Store every completed feature as an individual Git commit with a descriptive message. Explicitly tell me when it is time to commit.

### 2. Project Directory Structure
You will build the following structure in `src/autonomous_navigation`:
- `msg/`: Custom ROS 2 interfaces.
- `include/autonomous_navigation/` & `src/`: C++ header and source files for core logic.
- `test/`: GTest files for all C++ code.
- `launch/`: ROS 2 Python launch files (`auto_slam.launch.py`, `navigation.launch.py`).
- `config/`: YAML parameter files for SLAM Toolbox, Nav2, and Explore Lite.
- `worlds/` & `models/`: Gazebo simulation assets (e.g., Office, Warehouse).
- `maps/`: Saved `.pgm` and `.yaml` maps, and SLAM serialized checkpoints.
- `scripts/`: Shell scripts (e.g., `save_map.sh` using Nav2 map saver).
- `GUI Design/`: Front-end PyQt5 application.

### 3. Step-by-Step Implementation Plan

**Step 1: Custom Messages & Core Nav2 Action Client (C++)**
- Define any custom ROS 2 messages needed for state communication.
- Implement a C++ node that acts as an Action Client to `nav2_msgs::action::NavigateToPose`.
- Implement robust pause/resume capabilities using Nav2's cancel-and-replace trajectory mechanism.

**Step 2: Comprehensive Unit Testing (GTest)**
- Write highly granular GTest suites for all the C++ logic created in Step 1.
- Ensure the node builds and passes all tests using `ament_cmake_gtest`. Do NOT skip testing.

**Step 3: Gazebo Simulation Environments**
- Create at least two distinct custom Gazebo environments (e.g., an Office and a Warehouse).
- Ensure models load correctly using package-relative paths.

**Step 4: Mapping & Frontier Exploration Launch (`auto_slam.launch.py`)**
- Create a launch file that brings up: Gazebo, Robot State Publisher, SLAM Toolbox (online async), Nav2 components (for path planning to frontiers), and `explore_lite` (for autonomous frontier exploration).
- Document how to pause/resume mapping using SLAM Toolbox serialization.
- Write a bash script `scripts/save_map.sh` utilizing `nav2_map_server map_saver_cli`.

**Step 5: Autonomous Navigation Launch (`navigation.launch.py`)**
- Create a launch file that loads a pre-saved static map (`.yaml`/`.pgm`).
- Bring up AMCL for localization and Nav2 for costmaps and trajectory planning.

**Step 6: PyQt5 Modern GUI Development**
- Build a Python-based GUI in `GUI Design/main_window.py`.
- **Design Aesthetic ("Swiss" Design):** Must feel extremely premium. Pure White (`#ffffff`) main canvas, light gray background (`#F0F2F4`), Dark Charcoal (`#1A1D21`) typography. Strictly NO extraneous OS-style icons. Use micro-animations, smooth hover effects, and a drop shadow over the off-white background.
- **QStackedWidget Flow (Strictly 3 Pages):**
  - **Page 1 (Initialization State):** Heading: "Map Selection". Centered question: "On which map you want to navigate the Bot". Component: Single dropdown menu "Select Map" (White bg, light border, Vibrant Teal chevron).
  - **Page 2 (Mode Selection State):** Heading: Dynamic Map Name (e.g. "Map: Factory_Floor_1"). Centered question: "Do you want to do navigation or Mapping". Components: Two side-by-side buttons. Left: "Autonomous Navigation" (Vibrant Teal `#0D9488`), Right: "SLAM Mapping" (Muted Sage Green `#6B8F71`). Include a Universal Back button (white with light gray border) in the top left.
  - **Page 3 (Active Process State):** Heading: Dynamic Mode Name (e.g. "MANUAL SLAM MAPPING - MAP: FACTORY"). Components: A minimalist screen displaying large instructional text describing what the user must do (e.g., drive via terminal, click nav goal in RViz) and a large red "Stop Process" button to return to Page 2.

**Step 7: Final Integration & Polish**
- Integrate the GUI so that clicking the buttons programmatically triggers the respective ROS 2 launch files via Python's `subprocess` or `ros2 launch` API.
- Ensure the master launch file and overall workflow is flawless.

Please begin with Step 1 and provide the code for `package.xml`, `CMakeLists.txt`, and the Custom Messages. Do not proceed to Step 2 until I approve Step 1.
