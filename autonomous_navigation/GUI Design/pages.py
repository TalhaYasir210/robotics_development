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
        
        lbl_head = QLabel("On which map do you want to navigate the Bot?")
        lbl_head.setObjectName("mainHeading")
        lbl_head.setAlignment(Qt.AlignCenter)
        
        lbl_sub = QLabel("Choose from the pre-loaded maps available on this robot")
        lbl_sub.setObjectName("subHeading")
        lbl_sub.setAlignment(Qt.AlignCenter)
        
        self.combo = QComboBox()
        self.combo.setView(QListView()) # For proper QSS styling of dropdown list
        self.combo.addItems(["Select Map", "Office", "Warehouse"])
        self.combo.setCursor(QCursor(Qt.PointingHandCursor))
        
        self.btn_confirm = QPushButton("Confirm Map →")
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
        if text != "Select Map":
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
        
        lbl1 = QLabel("Active map:")
        lbl1.setObjectName("activeMapPrefix")
        self.lbl_map_name = QLabel("Unknown")
        self.lbl_map_name.setObjectName("activeMapValue")
        
        map_info_layout.addWidget(lbl1)
        map_info_layout.addWidget(self.lbl_map_name)
        
        self.main_layout.addWidget(lbl_head, alignment=Qt.AlignHCenter)
        self.main_layout.addLayout(map_info_layout)
        
        btn_layout = QHBoxLayout()
        btn_layout.setSpacing(20)
        btn_layout.setAlignment(Qt.AlignTop | Qt.AlignHCenter)
        
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
        self.slam_menu.setFixedWidth(380)
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

def create_feed_panel(header_text: str, overlays_left: list, overlays_right: list) -> QFrame:
    card = QFrame()
    card.setObjectName("feedPanelCard")
    card.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
    
    layout = QVBoxLayout(card)
    layout.setContentsMargins(0, 0, 0, 0)
    layout.setSpacing(0)
    
    header = QLabel(header_text)
    header.setObjectName("feedPanelHeader")
    layout.addWidget(header)
    
    feed_area = QFrame()
    feed_area.setObjectName("feedArea")
    layout.addWidget(feed_area, stretch=1)
    
    # Simple absolute positioning for tags
    def apply_tags():
        for tag_data in overlays_left:
            lbl = QLabel(tag_data["text"], feed_area)
            lbl.setProperty("class", tag_data.get("class", "overlayTag"))
            lbl.move(12, feed_area.height() - 30 if tag_data.get("pos") == "bottom" else 12)
            
        for tag_data in overlays_right:
            lbl = QLabel(tag_data["text"], feed_area)
            lbl.setProperty("class", tag_data.get("class", "overlayTag"))
            lbl.adjustSize()
            lbl.move(feed_area.width() - lbl.width() - 12, 
                     feed_area.height() - 30 if tag_data.get("pos") == "bottom" else 12)
    
    # Hack to reposition after layout
    QTimer = __import__("PyQt5.QtCore", fromlist=["QTimer"]).QTimer
    QTimer.singleShot(100, apply_tags)
    
    return card

class NavigationActionPage(BasePage):
    pause_clicked = pyqtSignal()
    cancel_clicked = pyqtSignal()

    def __init__(self):
        super().__init__()
        
        prompt = QLabel("Set the destination pose by clicking <b>2D Nav Goal</b> in the RViz2 panel")
        prompt.setObjectName("subHeadingSmall")
        prompt.setAlignment(Qt.AlignCenter)
        self.main_layout.addWidget(prompt)
        
        feed_layout = QHBoxLayout()
        feed_layout.setSpacing(20)
        
        p1 = create_feed_panel("MAP VIEW — RVIZ2", 
                               [{"text": "RViz2 - /map", "pos": "bottom"}], [])
        p2 = create_feed_panel("LIVE CAMERA — GAZEBO", 
                               [{"text": "vel_x: 0.50 m/s", "pos": "top"}, {"text": "Gazebo - /camera/rgb", "pos": "bottom"}], 
                               [{"text": "● LIVE - 30 HZ", "class": "overlayTagRed", "pos": "top"}])
        feed_layout.addWidget(p1)
        feed_layout.addWidget(p2)
        
        self.main_layout.addLayout(feed_layout, stretch=1)
        
        btn_layout = QHBoxLayout()
        btn_layout.setSpacing(16)
        btn_layout.setContentsMargins(0, 24, 0, 0)
        btn_layout.setAlignment(Qt.AlignHCenter)
        
        self.btn_pause = QPushButton("Pause Navigation")
        self.btn_pause.setObjectName("btnPause")
        self.btn_pause.setCursor(QCursor(Qt.PointingHandCursor))
        self.btn_pause.clicked.connect(self.pause_clicked.emit)
        
        self.btn_cancel = QPushButton("Cancel Navigation")
        self.btn_cancel.setObjectName("btnCancel")
        self.btn_cancel.setCursor(QCursor(Qt.PointingHandCursor))
        self.btn_cancel.clicked.connect(self.cancel_clicked.emit)
        
        btn_layout.addWidget(self.btn_pause)
        btn_layout.addWidget(self.btn_cancel)
        self.main_layout.addLayout(btn_layout)

class AutoMappingPage(BasePage):
    def __init__(self):
        super().__init__()
        
        prompt = QLabel("Robot is autonomously exploring and building the map — no operator input required")
        prompt.setObjectName("subHeadingSmall")
        prompt.setAlignment(Qt.AlignCenter)
        self.main_layout.addWidget(prompt)
        
        feed_layout = QHBoxLayout()
        feed_layout.setSpacing(20)
        
        p1 = create_feed_panel("MAP VIEW — RVIZ2 / SLAM (BUILDING)", 
                               [{"text": "RViz2 - /map", "pos": "bottom"}], 
                               [{"text": "28x22 · 0.05m/px", "pos": "top"}])
        p2 = create_feed_panel("LIVE CAMERA — GAZEBO", 
                               [{"text": "vel_x: 0.50 m/s", "pos": "top"}, {"text": "Gazebo - /camera/rgb", "pos": "bottom"}], 
                               [{"text": "● LIVE - 30 HZ", "class": "overlayTagRed", "pos": "top"}])
        feed_layout.addWidget(p1)
        feed_layout.addWidget(p2)
        
        self.main_layout.addLayout(feed_layout, stretch=1)

class ManualMappingPage(BasePage):
    def __init__(self):
        super().__init__()
        
        prompt = QLabel("Drive the robot manually using keyboard controls to build the map")
        prompt.setObjectName("subHeadingSmall")
        prompt.setAlignment(Qt.AlignCenter)
        self.main_layout.addWidget(prompt)
        
        feed_layout = QHBoxLayout()
        feed_layout.setSpacing(20)
        
        p1 = create_feed_panel("MAP VIEW — RVIZ2 / SLAM (BUILDING)", 
                               [{"text": "RViz2 - /map", "pos": "bottom"}], 
                               [{"text": "28x22 · 0.05m/px", "pos": "top"}])
        p2 = create_feed_panel("LIVE CAMERA — GAZEBO", 
                               [{"text": "vel_x: 0.50 m/s", "pos": "top"}, {"text": "Gazebo - /camera/rgb", "pos": "bottom"}], 
                               [{"text": "● LIVE - 30 HZ", "class": "overlayTagRed", "pos": "top"}])
        feed_layout.addWidget(p1)
        feed_layout.addWidget(p2)
        
        self.main_layout.addLayout(feed_layout, stretch=1)
        
        # Teleop Panel
        kb_box = QFrame()
        kb_box.setObjectName("teleopContainer")
        kb_layout = QVBoxLayout(kb_box)
        kb_layout.setContentsMargins(0,0,0,0)
        kb_layout.setAlignment(Qt.AlignTop | Qt.AlignHCenter)
        
        kb_title = QLabel("TELEOP KEYBOARD CONTROLS")
        kb_title.setObjectName("teleopTitle")
        kb_title.setAlignment(Qt.AlignCenter)
        kb_layout.addWidget(kb_title)
        kb_layout.addSpacing(16)
        
        grid = QGridLayout()
        grid.setSpacing(8)
        grid.setAlignment(Qt.AlignCenter)
        
        def make_key(letter, text):
            v = QVBoxLayout()
            v.setAlignment(Qt.AlignCenter)
            v.setSpacing(6)
            
            btn = QLabel(letter)
            btn.setObjectName("keyBtn")
            btn.setAlignment(Qt.AlignCenter)
            
            # Drop shadow
            shadow = QGraphicsDropShadowEffect()
            shadow.setBlurRadius(4)
            shadow.setXOffset(0)
            shadow.setYOffset(2)
            shadow.setColor(QColor(0, 0, 0, 40))
            btn.setGraphicsEffect(shadow)
            
            lbl = QLabel(text)
            lbl.setObjectName("keyLabel")
            lbl.setAlignment(Qt.AlignCenter)
            
            v.addWidget(btn, alignment=Qt.AlignHCenter)
            v.addWidget(lbl, alignment=Qt.AlignHCenter)
            w = QWidget()
            w.setLayout(v)
            return w
            
        grid.addWidget(make_key("I", "Move Forward"), 0, 1)
        grid.addWidget(make_key("J", "Rotate Left"), 1, 0)
        grid.addWidget(make_key("K", "Stop"), 1, 1)
        grid.addWidget(make_key("L", "Rotate Right"), 1, 2)
        
        kb_layout.addLayout(grid)
        kb_layout.addSpacing(16)
        
        self.main_layout.addSpacing(24)
        self.main_layout.addWidget(kb_box, alignment=Qt.AlignHCenter)
