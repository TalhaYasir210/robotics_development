import sys
import os

# Fix for FastRTPS service timeouts in ROS 2 Jazzy (causes instant failure for Nav2 lifecycle manager and action clients)
# Using localhost only prevents UDP multicast discovery spam which overwhelms the default middleware buffers on a single PC.
os.environ["ROS_LOCALHOST_ONLY"] = "1"

import subprocess
import signal
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
    QStackedWidget, QTextEdit, QPushButton, QLabel, QShortcut, QFrame, QGridLayout
)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QKeySequence, QFont, QColor, QTextCharFormat, QTextCursor

from pages import (
    InitializationPage, ModeSelectionPage, ActiveProcessPage
)

class LoadingOverlay(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAttribute(Qt.WA_TransparentForMouseEvents, False) # block clicks
        self.setStyleSheet("background-color: rgba(240, 242, 244, 230);")
        
        self.lbl_text = QLabel("Wait, processes are being closed", self)
        self.lbl_text.setStyleSheet("color: red; font-weight: bold; font-size: 24px; background: transparent;")
        self.lbl_text.setAlignment(Qt.AlignCenter)
        self.hide()
        
        self.dots = 0
        self.anim_timer = QTimer(self)
        self.anim_timer.timeout.connect(self.update_text)
        
    def show_overlay(self, ref_widget=None):
        if self.parent():
            self.resize(self.parent().size())
            
        self.dots = 0
        self.lbl_text.setText("Wait, processes are being closed")
        self.anim_timer.start(500)
        
        self.lbl_text.setFixedWidth(self.width())
        if ref_widget and ref_widget.isVisible():
            global_pos = ref_widget.mapToGlobal(ref_widget.rect().bottomLeft())
            local_pos = self.mapFromGlobal(global_pos)
            self.lbl_text.move(0, local_pos.y() + 20)
        else:
            self.lbl_text.move(0, self.height() // 2 + 80)
            
        self.show()
        self.raise_()
        
    def hide_overlay(self):
        self.anim_timer.stop()
        self.hide()
        
    def update_text(self):
        self.dots = (self.dots + 1) % 4
        self.lbl_text.setText("Wait, processes are being closed" + "." * self.dots)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("ROS 2 Autonomous Navigation / Mapping")
        
        # Load Stylesheet
        qss_path = os.path.join(os.path.dirname(__file__), "theme.qss")
        if os.path.exists(qss_path):
            with open(qss_path, "r") as f:
                self.setStyleSheet(f.read())

        self.shortcut_esc = QShortcut(QKeySequence("Esc"), self)
        self.shortcut_esc.activated.connect(self.close)

        self.central_widget = QWidget()
        self.central_widget.setObjectName("mainCanvas")
        self.setCentralWidget(self.central_widget)
        
        self.main_layout = QVBoxLayout(self.central_widget)
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.main_layout.setSpacing(0)
        
        # ================== Section 1: Top Navigation Bar ==================
        self.header_bar = QFrame()
        self.header_bar.setObjectName("headerBar")
        self.header_layout = QGridLayout(self.header_bar)
        self.header_layout.setContentsMargins(24, 0, 24, 0)
        
        # Left (Back Button)
        self.btn_back = QPushButton("‹ Back")
        self.btn_back.setObjectName("btnBack")
        self.btn_back.clicked.connect(self.go_back)
        self.btn_back.hide() # Hidden on first page
        
        # Center (Title)
        self.lbl_title = QLabel("ENVIRONMENT SELECTION")
        self.lbl_title.setObjectName("headerTitle")
        self.lbl_title.setAlignment(Qt.AlignCenter)
        
        # Right (Badges)
        self.pill_layout = QHBoxLayout()
        self.pill_layout.setSpacing(10)
        self.pill_layout.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        
        self.pill_status = QLabel("")
        self.pill_status.hide()
        
        self.pill_ros = QLabel("ROS 2 Jazzy")
        self.pill_ros.setProperty("class", "statusBadge")
        
        self.pill_layout.addWidget(self.pill_status)
        self.pill_layout.addWidget(self.pill_ros)
        
        # Add to header layout using Grid (row 0, columns 0, 1, 2)
        self.header_layout.addWidget(self.btn_back, 0, 0, Qt.AlignLeft | Qt.AlignVCenter)
        self.header_layout.addWidget(self.lbl_title, 0, 1, Qt.AlignCenter)
        self.header_layout.addLayout(self.pill_layout, 0, 2, Qt.AlignRight | Qt.AlignVCenter)
        
        # Equal stretch on columns 0 and 2 keeps column 1 perfectly centered!
        self.header_layout.setColumnStretch(0, 1)
        self.header_layout.setColumnStretch(1, 0)
        self.header_layout.setColumnStretch(2, 1)
        
        self.main_layout.addWidget(self.header_bar)
        
        # ================== Section 2: Main Content Area ==================
        self.stacked_widget = QStackedWidget()
        self.main_layout.addWidget(self.stacked_widget)
        
        # Initialize Pages
        self.page_init = InitializationPage()
        self.page_mode = ModeSelectionPage()
        self.page_active = ActiveProcessPage()
        
        self.stacked_widget.addWidget(self.page_init)       # Index 0
        self.stacked_widget.addWidget(self.page_mode)       # Index 1
        self.stacked_widget.addWidget(self.page_active)     # Index 2
        
        # Set exact stretch ratios for a responsive layout
        self.main_layout.setStretchFactor(self.header_bar, 0)         # Fit to content
        self.main_layout.setStretchFactor(self.stacked_widget, 1)     # Fill remaining height
        
        # ================== Signals & State ==================
        self.page_init.map_selected.connect(self.on_map_selected)
        self.page_mode.nav_selected.connect(lambda: self.switch_page(2, "nav"))
        self.page_mode.auto_map_selected.connect(lambda: self.switch_page(2, "auto_map"))
        self.page_mode.manual_map_selected.connect(lambda: self.switch_page(2, "manual_map"))
        # self.page_active.stop_clicked.connect(self.go_back)  # Removed stop button
        
        self.history = []
        self.current_map = ""
        self.current_process = None
        self.teleop_process = None
        self.env_process = None
        self.shutdown_timer = None
        
        self.loading_overlay = LoadingOverlay(self.central_widget)
        # every 1sec the GUi checks on ros2 process, if env_process suddenly stops responding , check_process_status function can catch it and update the GUI 
        self.poll_timer = QTimer(self)
        self.poll_timer.timeout.connect(self.check_process_status)
        self.poll_timer.start(1000)
        
    def switch_page(self, index, mode=None):
        current = self.stacked_widget.currentIndex()
        self.history.append(current)
        self.stacked_widget.setCurrentIndex(index)
        
        if index == 2:
            self.btn_back.hide()
        else:
            self.btn_back.show()
        
        self.pill_status.hide()
        
        if index == 1:
            self.lbl_title.setText(f"ENVIRONMENT: {self.current_map.upper()}")
        elif index == 2:
            self.page_active.set_state(mode)
            if mode == "nav":
                self.lbl_title.setText(f"AUTONOMOUS NAVIGATION - ENVIRONMENT: {self.current_map.upper()}")
                self.pill_status.setText("NAV ACTIVE")
                self.pill_status.setProperty("class", "statusBadge")
                map_path = f"/home/deadsec/ros2_ws/src/robotics_development/autonomous_navigation/maps/my_{self.current_map}_map.yaml"
                cmd = ["ros2", "launch", "autonomous_navigation", "navigation.launch.py", f"env:={self.current_map}", f"map:={map_path}"]
                terminal_cmd = ["gnome-terminal", "--wait", "--"] + cmd
                self.current_process = subprocess.Popen(terminal_cmd)
            elif mode == "auto_map":
                self.lbl_title.setText(f"AUTO SLAM MAPPING - ENVIRONMENT: {self.current_map.upper()}")
                self.pill_status.setText("MAPPING")
                self.pill_status.setProperty("class", "statusBadge")
                cmd = ["ros2", "launch", "autonomous_navigation", "auto_slam.launch.py", f"env:={self.current_map}", "mode:=auto"]
                terminal_cmd = ["gnome-terminal", "--wait", "--"] + cmd
                self.current_process = subprocess.Popen(terminal_cmd)
            elif mode == "manual_map":
                self.lbl_title.setText(f"MANUAL SLAM MAPPING - ENVIRONMENT: {self.current_map.upper()}")
                self.pill_status.setText("TELEOP")
                self.pill_status.setProperty("class", "statusBadgeTeleop")
                cmd = ["ros2", "launch", "autonomous_navigation", "auto_slam.launch.py", f"env:={self.current_map}", "mode:=manual"]
                terminal_cmd = ["gnome-terminal", "--wait", "--"] + cmd
                self.current_process = subprocess.Popen(terminal_cmd)
                
                teleop_cmd = ["gnome-terminal", "--wait", "--", "ros2", "run", "teleop_twist_keyboard", "teleop_twist_keyboard", "--ros-args", "--remap", "cmd_vel:=cmd_vel_unstamped"]
                self.teleop_process = subprocess.Popen(teleop_cmd)
                
            self.pill_status.show()
            
        self.style().unpolish(self.pill_status)
        self.style().polish(self.pill_status)
        
    def check_process_status(self):
        if self.current_process and self.current_process.poll() is not None:
            # Process has terminated (e.g., user closed RViz)
            # Only go back if we are currently on the ActiveProcessPage (Index 2)
            if self.stacked_widget.currentIndex() == 2:
                self.go_back()

    def shutdown_all(self, callback, kill_env_too=False):
        if self.current_process:
            try:
                self.current_process.send_signal(signal.SIGINT)
                #subprocess.run(["pkill", "-f", "auto_slam.launch.py"])
                #subprocess.run(["pkill", "-f", "navigation.launch.py"])
            except Exception:
                pass
                
        if self.teleop_process:
            try:
                self.teleop_process.send_signal(signal.SIGINT)
                #subprocess.run(["pkill", "-f", "teleop_twist_keyboard"])
            except Exception:
                pass
                
        if kill_env_too and self.env_process:
            try:
                self.env_process.send_signal(signal.SIGINT)
            except Exception:
                pass

        if self.stacked_widget.currentWidget() != self.page_init:
            self.loading_overlay.show_overlay()
        self.shutdown_ticks = 0
        
        self.shutdown_timer = QTimer(self)
        
        def check_shutdown():
            self.shutdown_ticks += 1
            all_dead = True
            
            if self.current_process and self.current_process.poll() is None:
                all_dead = False
                
            if kill_env_too:
                try:
                    # Check if Gazebo or robot_state_publisher are still lingering in the background
                    gz_check = subprocess.run(["pgrep", "-f", "gz sim|ruby|robot_state_publisher"], stdout=subprocess.PIPE)
                    if gz_check.returncode == 0:  # 0 means matches were found!
                        all_dead = False
                except Exception:
                    pass
                
            if all_dead or self.shutdown_ticks > 20: # 10 seconds max graceful limit
                self.shutdown_timer.stop()
                self.loading_overlay.hide_overlay()
                
                # Safety net: Guarantee everything is absolutely dead to prevent TF clock clashes
                if kill_env_too:
                    subprocess.run(["pkill", "-9", "-f", "gz sim"])
                    subprocess.run(["pkill", "-9", "-f", "ruby"])
                    subprocess.run(["pkill", "-9", "-f", "robot_state_publisher"])
                
                subprocess.run(["pkill", "-9", "-f", "teleop_twist_keyboard"])
                subprocess.run(["pkill", "-9", "-f", "rviz2"])
                subprocess.run(["pkill", "-9", "-f", "slam_toolbox"])
                subprocess.run(["pkill", "-9", "-f", "nav2"])
                subprocess.run(["pkill", "-9", "-f", "explore"])
                subprocess.run(["pkill", "-9", "-f", "amcl"])
                subprocess.run(["pkill", "-9", "-f", "component_container"])
                    
                self.current_process = None
                self.teleop_process = None
                if kill_env_too:
                    self.env_process = None
                    
                callback()
                
        self.shutdown_timer.timeout.connect(check_shutdown)
        self.shutdown_timer.start(500)

    def go_back(self):
        if self.history:
            prev = self.history.pop()
            
            def complete_transition():
                self.stacked_widget.setCurrentIndex(prev)
                if prev == 0:
                    self.btn_back.hide()
                    self.lbl_title.setText("ENVIRONMENT SELECTION")
                    self.pill_status.hide()
                    self.current_map = ""
                elif prev == 1:
                    self.btn_back.show()
                    self.lbl_title.setText(f"ENVIRONMENT: {self.current_map.upper()}")
                    self.pill_status.hide()
                    
            kill_env_too = (prev == 0)
            self.shutdown_all(callback=complete_transition, kill_env_too=kill_env_too)
            
    def on_map_selected(self, map_name):
        new_map = map_name.lower()
        
        # Only relaunch environment if the map actually changed
        if self.current_map != new_map:
            def complete_map_change():
                self.current_map = new_map
                env_cmd = ["ros2", "launch", "autonomous_navigation", f"{self.current_map}_env.launch.py"]
                self.env_process = subprocess.Popen(env_cmd)
                # Give Gazebo time to start before launching Nav2 (avoid instant clock timeouts)
                import time
                time.sleep(12)
                self.page_mode.set_map_name(map_name)
                self.switch_page(1)
                
            self.shutdown_all(callback=complete_map_change, kill_env_too=True)
        else:
            self.page_mode.set_map_name(map_name)
            self.switch_page(1)
        
    def closeEvent(self, event):
        if self.current_process:
            self.current_process.send_signal(signal.SIGINT)
        if self.teleop_process:
            try:
                self.teleop_process.send_signal(signal.SIGINT)
            except:
                pass
            subprocess.run(["pkill", "-f", "teleop_twist_keyboard"])
        if self.env_process:
            self.env_process.send_signal(signal.SIGINT)
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MainWindow()
    
    # Handle Ctrl+C gracefully from the terminal
    def sigint_handler(*args):
        window.close()
    signal.signal(signal.SIGINT, sigint_handler)
    
    # QTimer is needed to let the Python interpreter catch the signal in the Qt event loop
    timer = QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)
    
    window.resize(1152, 648)
    window.show()
    sys.exit(app.exec_())
