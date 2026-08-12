import typing
from PyQt5.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QPushButton, 
    QComboBox, QSpacerItem, QSizePolicy, QGraphicsDropShadowEffect,
    QFrame, QGridLayout, QListView
)
from PyQt5.QtCore import Qt, pyqtSignal
from PyQt5.QtGui import QColor, QFont, QCursor

class BasePage(QWidget):
    def __init__(self):
        super().__init__()
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setContentsMargins(24, 24, 24, 24)

class InitializationPage(BasePage):
    map_selected = pyqtSignal(str)

    def __init__(self):
        super().__init__()
        self.main_layout.setAlignment(Qt.AlignCenter)
        
        lbl_head = QLabel("Which simulation environment do you want to launch?")
        lbl_head.setObjectName("mainHeading")
        lbl_head.setAlignment(Qt.AlignCenter)
        
        lbl_sub = QLabel("Choose from the pre-loaded simulation environments available on this robot")
        lbl_sub.setObjectName("subHeading")
        lbl_sub.setAlignment(Qt.AlignCenter)
        
        self.combo = QComboBox()
        self.combo.setView(QListView()) # For proper QSS styling of dropdown list
        self.combo.addItems(["Select Environment", "Office", "Warehouse"])
        self.combo.setCursor(QCursor(Qt.PointingHandCursor))
        
        self.btn_confirm = QPushButton("Confirm Environment →")
        self.btn_confirm.setObjectName("confirmMapBtn")
        self.btn_confirm.setCursor(QCursor(Qt.PointingHandCursor))
        sp = self.btn_confirm.sizePolicy()
        sp.setRetainSizeWhenHidden(True)
        self.btn_confirm.setSizePolicy(sp)
        self.btn_confirm.hide()
        
        self.combo.currentTextChanged.connect(self.on_map_changed)
        self.btn_confirm.clicked.connect(self.on_confirm)
        
        self.main_layout.addWidget(lbl_head, alignment=Qt.AlignHCenter)
        self.main_layout.addWidget(lbl_sub, alignment=Qt.AlignHCenter)
        self.main_layout.addWidget(self.combo, alignment=Qt.AlignHCenter)
        self.main_layout.addWidget(self.btn_confirm, alignment=Qt.AlignHCenter)
        
    def on_map_changed(self, text):
        if text != "Select Environment":
            self.btn_confirm.show()
        else:
            self.btn_confirm.hide()
            
    def on_confirm(self):
        self.map_selected.emit(self.combo.currentText())

class ModeSelectionPage(BasePage):
    nav_selected = pyqtSignal()
    auto_map_selected = pyqtSignal()
    manual_map_selected = pyqtSignal()

    def __init__(self):
        super().__init__()
        self.main_layout.setAlignment(Qt.AlignCenter)
        
        lbl_head = QLabel("Do you want to do Navigation or Mapping?")
        lbl_head.setObjectName("mainHeading")
        lbl_head.setAlignment(Qt.AlignCenter)
        
        map_info_layout = QHBoxLayout()
        map_info_layout.setAlignment(Qt.AlignCenter)
        map_info_layout.setSpacing(8)
        
        lbl1 = QLabel("Active world:")
        lbl1.setObjectName("activeMapPrefix")
        self.lbl_map_name = QLabel("Unknown")
        self.lbl_map_name.setObjectName("activeMapValue")
        
        map_info_layout.addWidget(lbl1)
        map_info_layout.addWidget(self.lbl_map_name)
        
        self.main_layout.addWidget(lbl_head, alignment=Qt.AlignHCenter)
        self.main_layout.addLayout(map_info_layout)
        
        btn_layout = QHBoxLayout()
        btn_layout.setSpacing(20)
        btn_layout.setAlignment(Qt.AlignCenter)
        
        self.btn_nav = QPushButton("Autonomous Navigation")
        self.btn_nav.setObjectName("btnNavigation")
        self.btn_nav.setCursor(QCursor(Qt.PointingHandCursor))
        self.btn_nav.clicked.connect(self.nav_selected.emit)
        btn_layout.addWidget(self.btn_nav, alignment=Qt.AlignTop)
        
        # SLAM Mapping Column
        slam_layout = QVBoxLayout()
        slam_layout.setSpacing(0)
        slam_layout.setAlignment(Qt.AlignTop)
        
        self.btn_slam = QPushButton("SLAM Mapping v")
        self.btn_slam.setObjectName("btnSlamMapping")
        self.btn_slam.setCursor(QCursor(Qt.PointingHandCursor))
        self.btn_slam.clicked.connect(self.toggle_slam_menu)
        slam_layout.addWidget(self.btn_slam)
        
        # Spacer for 4px gap
        slam_layout.addSpacing(4)
        
        self.slam_menu = QFrame()
        self.slam_menu.setObjectName("slamMenuBox")
        self.slam_menu.setFixedWidth(260)
        sp2 = self.slam_menu.sizePolicy()
        sp2.setRetainSizeWhenHidden(True)
        self.slam_menu.setSizePolicy(sp2)
        menu_v = QVBoxLayout(self.slam_menu)
        menu_v.setContentsMargins(0, 0, 0, 0)
        menu_v.setSpacing(0)
        
        def create_menu_item(title, desc, is_last=False):
            w = QFrame()
            w.setObjectName("slamMenuItem")
            w.setCursor(QCursor(Qt.PointingHandCursor))
            v = QVBoxLayout(w)
            v.setContentsMargins(16, 12, 16, 12)
            v.setSpacing(4)
            
            t = QLabel(title)
            t.setObjectName("slamMenuTitle")
            d = QLabel(desc)
            d.setObjectName("slamMenuDesc")
            
            v.addWidget(t)
            v.addWidget(d)
            return w
            
        self.item_auto = create_menu_item("Auto Mapping", "Autonomous frontier exploration")
        self.item_auto.mousePressEvent = lambda e: self.auto_map_selected.emit()
        menu_v.addWidget(self.item_auto)
        
        sep = QFrame()
        sep.setObjectName("slamMenuSeparator")
        menu_v.addWidget(sep)
        
        self.item_manual = create_menu_item("Manual Mapping", "Teleoperate with keyboard")
        self.item_manual.mousePressEvent = lambda e: self.manual_map_selected.emit()
        menu_v.addWidget(self.item_manual)
        
        self.slam_menu.hide()
        slam_layout.addWidget(self.slam_menu)
        
        btn_layout.addLayout(slam_layout)
        self.main_layout.addLayout(btn_layout)
        
    def set_map_name(self, name: str):
        self.lbl_map_name.setText(name)
        
    def toggle_slam_menu(self):
        if self.slam_menu.isHidden():
            self.slam_menu.show()
            self.btn_slam.setText("SLAM Mapping ^")
        else:
            self.slam_menu.hide()
            self.btn_slam.setText("SLAM Mapping v")

class ActiveProcessPage(BasePage):
    stop_clicked = pyqtSignal()

    def __init__(self):
        super().__init__()
        
        self.lbl_instructions = QLabel("Process instructions will appear here.")
        self.lbl_instructions.setObjectName("instructionText")
        self.lbl_instructions.setAlignment(Qt.AlignCenter)
        self.lbl_instructions.setWordWrap(True)
        self.main_layout.addWidget(self.lbl_instructions, stretch=1)
        
    def set_state(self, mode: str):
        if mode == "nav":
            self.lbl_instructions.setText(
                "Focus on the RViz window and click <b>2D Nav Goal</b> to set a destination pose for the robot.<br><br>"
                "Close RViz to finish and return to the menu."
            )
        elif mode == "auto_map":
            self.lbl_instructions.setText(
                "Frontier Exploration is actively mapping the environment.<br><br>"
                "Monitor the progress in RViz. Close RViz to finish."
            )
        elif mode == "manual_map":
            self.lbl_instructions.setText(
                "Focus on the terminal where you launched this application and use the<br>"
                "<b>I, J, K, L</b> keys to drive the robot.<br><br>"
                "Close RViz to finish."
            )


