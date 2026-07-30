import { useState, useEffect, useRef, useCallback } from "react";
import { ChevronDown, ChevronLeft, ArrowRight } from "lucide-react";

// ─── Global fade + click animation styles ─────────────────────────
const GLOBAL_CSS = `
@keyframes ros-fadein {
  from { opacity: 0; transform: translateY(6px); }
  to   { opacity: 1; transform: translateY(0);   }
}
.ros-page {
  animation: ros-fadein 0.28s cubic-bezier(0.22,1,0.36,1) both;
}
.ros-btn {
  transition: opacity 0.14s, transform 0.10s;
}
.ros-btn:hover  { opacity: 0.88; }
.ros-btn:active { transform: scale(0.965); opacity: 0.75; }
.ros-back:hover  { background: #F5F6F8 !important; }
.ros-back:active { transform: scale(0.96); }
.ros-dd-item:hover { background: #F5F6F8 !important; }
`;

// ─── Design tokens ─────────────────────────────────────────────────
const T = {
  teal:     "#0D9488",
  tealLight:"#E6F4F2",
  sage:     "#5E8C65",
  sageLight:"#EBF3EC",
  blue:     "#5B7FD4",
  blueLight:"#EBF0FA",
  coral:    "#D96B6B",
  coralLight:"#FAEAEA",
  appBg:    "#ECEEF1",
  canvas:   "#FFFFFF",
  logBg:    "#E8EAED",
  logStripe:"#E2E4E8",
  border:   "rgba(0,0,0,0.09)",
  borderMed:"rgba(0,0,0,0.14)",
  ink:      "#1C1F24",
  dim:      "#5A6272",
  dimLight: "#8A93A2",
  mapFree:  "#F0F4F8",
  mapWall:  "#2C3340",
  mapUnknown:"#C8CDD6",
};

type Page = "init" | "mode" | "nav" | "automap" | "manualmap";

// ─── Timestamp helper ──────────────────────────────────────────────
function ts() {
  const d = new Date();
  return `[${String(d.getHours()).padStart(2,"0")}:${String(d.getMinutes()).padStart(2,"0")}:${String(d.getSeconds()).padStart(2,"0")}.${String(d.getMilliseconds()).padStart(3,"0")}]`;
}

// ─── Log pools ─────────────────────────────────────────────────────
const RAW_LOGS: Record<Page, Array<{ level: "INFO"|"DEBUG"|"WARN"|"ERROR"; msg: string }>> = {
  init: [
    { level:"INFO",  msg:"ros2_node           Initializing ROS 2 middleware (RMW: FastRTPS)" },
    { level:"INFO",  msg:"map_server           Scanning /maps directory for YAML descriptors" },
    { level:"DEBUG", msg:"param_server         Loaded 47 parameters from ros2_params.yaml" },
    { level:"INFO",  msg:"tf2_ros              Static transform broadcaster started" },
    { level:"DEBUG", msg:"map_server           Discovered: office.yaml (512×512, 0.05m/px)" },
    { level:"DEBUG", msg:"map_server           Discovered: warehouse.yaml (768×512, 0.05m/px)" },
    { level:"INFO",  msg:"nav2_lifecycle       All lifecycle nodes set to INACTIVE state" },
    { level:"DEBUG", msg:"dds_layer            ROS_DOMAIN_ID=0  |  Participant GUID resolved" },
    { level:"INFO",  msg:"robot_state_pub      Joint states publisher ready on /joint_states" },
  ],
  mode: [
    { level:"INFO",  msg:"map_server           Map loaded successfully — factory_floor_1 (512×512 @ 0.05 m/px)" },
    { level:"INFO",  msg:"amcl                 Initializing Monte-Carlo localization (N=500 particles)" },
    { level:"DEBUG", msg:"costmap_2d           Global costmap: 25.6×25.6 m, inflation_radius=0.30 m" },
    { level:"INFO",  msg:"nav2_bt_navigator    Behavior tree loaded: navigate_w_replanning.xml" },
    { level:"INFO",  msg:"slam_toolbox         Ready — awaiting mode selection from operator" },
    { level:"DEBUG", msg:"rviz2                Subscribed to /map, /scan, /amcl_pose topics" },
  ],
  nav: [
    { level:"INFO",  msg:"nav2_bt_navigator    Autonomous navigation mode ACTIVE" },
    { level:"INFO",  msg:"amcl                 Particle filter converged — estimated confidence: 0.947" },
    { level:"INFO",  msg:"rviz2                Waiting for 2D Nav Goal via RViz interactive tool..." },
    { level:"DEBUG", msg:"controller_server    DWB local planner initialized, max_vel_x=0.50 m/s" },
    { level:"DEBUG", msg:"sensor_fusion        /scan @ 10 Hz  |  /camera/rgb/image_raw @ 30 Hz" },
    { level:"INFO",  msg:"nav2_bt_navigator    Goal pose received: x=4.21  y=2.85  θ=0.00 rad" },
    { level:"INFO",  msg:"global_planner       Path computed: 47 waypoints, distance=12.3 m" },
    { level:"DEBUG", msg:"local_planner        Executing path segment 1/47 — obstacle-free corridor" },
    { level:"INFO",  msg:"nav2_bt_navigator    Robot in motion — estimated arrival: 24.6 s" },
  ],
  automap: [
    { level:"INFO",  msg:"slam_toolbox         Auto SLAM mapping mode ACTIVE" },
    { level:"INFO",  msg:"slam_toolbox         Serialization ON — output: /maps/slam_session_001" },
    { level:"DEBUG", msg:"laser_scan_matcher   ICP convergence achieved — avg residual: 0.003 m" },
    { level:"INFO",  msg:"occupancy_grid       Resolution 0.05 m/px  |  Origin [0.000, 0.000, 0.000]" },
    { level:"INFO",  msg:"explore_lite         Frontier exploration initiated" },
    { level:"DEBUG", msg:"explore_lite         Active frontiers detected: 14" },
    { level:"INFO",  msg:"move_base            Navigating to frontier [4.21 m, 1.87 m]" },
    { level:"DEBUG", msg:"slam_toolbox         Loop closure detected — optimising pose graph..." },
    { level:"INFO",  msg:"slam_toolbox         Pose graph updated: 128 nodes, 134 edges" },
    { level:"DEBUG", msg:"occupancy_grid       Map size grown to 340×298 cells" },
  ],
  manualmap: [
    { level:"INFO",  msg:"slam_toolbox         Manual SLAM mapping mode ACTIVE" },
    { level:"INFO",  msg:"teleop_twist_keyboard Ready — listening on /cmd_vel topic" },
    { level:"DEBUG", msg:"laser_scan_matcher   ICP convergence achieved — avg residual: 0.004 m" },
    { level:"INFO",  msg:"occupancy_grid       Resolution 0.05 m/px  |  Origin [0.000, 0.000, 0.000]" },
    { level:"INFO",  msg:"slam_toolbox         Serialization ON — output: /maps/manual_session_001" },
    { level:"DEBUG", msg:"teleop               Key [i] — cmd_vel: linear.x=0.50  angular.z=0.00" },
    { level:"DEBUG", msg:"teleop               Key [j] — cmd_vel: linear.x=0.00  angular.z=0.60" },
    { level:"INFO",  msg:"slam_toolbox         Pose graph updated: 23 nodes, 25 edges" },
  ],
};

// ─── Shared primitives ─────────────────────────────────────────────

function BackButton({ onClick }: { onClick: () => void }) {
  return (
    <button
      onClick={onClick}
      className="ros-back"
      style={{
        display:"flex", alignItems:"center", gap:6,
        padding:"6px 12px", borderRadius:6,
        background: T.canvas,
        border: `1px solid ${T.borderMed}`,
        color: T.ink,
        fontSize:13, fontWeight:500,
        fontFamily:"Inter,sans-serif",
        cursor:"pointer",
        transition:"background 0.14s, transform 0.10s",
      }}
    >
      <ChevronLeft size={14} strokeWidth={2.2} />
      Back
    </button>
  );
}

function Btn({
  label, bg, onClick, wide=false, icon
}: {
  label:string; bg:string; onClick?:()=>void; wide?:boolean; icon?: React.ReactNode
}) {
  return (
    <button
      onClick={onClick}
      className="ros-btn"
      style={{
        display:"flex", alignItems:"center", justifyContent:"center", gap:8,
        padding: wide ? "13px 36px" : "11px 24px",
        borderRadius:7,
        background: bg,
        color:"#fff",
        fontSize:14, fontWeight:600,
        fontFamily:"Inter,sans-serif",
        border:"none", cursor:"pointer",
        minWidth: wide ? 210 : undefined,
        letterSpacing:"0.01em",
      }}
    >
      {label}
      {icon}
    </button>
  );
}

// ─── Occupancy grid map ────────────────────────────────────────────
const ROWS=22, COLS=28;

function buildBaseMap(): number[][] {
  // 0=free, 1=wall, 2=unknown
  return Array.from({length:ROWS},(_,r)=>
    Array.from({length:COLS},(_,c)=>{
      if(r===0||r===ROWS-1||c===0||c===COLS-1) return 1;
      // inner walls
      if(r===7 && c>3 && c<12) return 1;
      if(r===15 && c>9 && c<22) return 1;
      if(c===14 && r>4 && r<11) return 1;
      if(r===11 && c>14 && c<20) return 1;
      if(r===4 && c===20) return 1;
      if(r===5 && c===20) return 1;
      if(r===18 && c>2 && c<8) return 1;
      return 0;
    })
  );
}

const BASE_MAP = buildBaseMap();

function OccupancyMap({ mode }: { mode: "nav"|"build" }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const stateRef = useRef({
    revealed: BASE_MAP.map(r=>r.map(_=>mode==="nav")),
    robot: {r:11,c:7,angle:0},
    goal: {r:5,c:21},
    path: [] as {r:number,c:number}[],
    tick: 0,
  });

  // generate a simple path for nav mode
  useEffect(()=>{
    if(mode==="nav"){
      const p: {r:number,c:number}[] = [];
      for(let i=0;i<=12;i++) p.push({r:11,c:7+i});
      for(let i=0;i<=6;i++) p.push({r:11-i,c:19});
      for(let i=0;i<=2;i++) p.push({r:5,c:19+i});
      stateRef.current.path = p;
    }
  },[mode]);

  useEffect(()=>{
    const canvas = canvasRef.current; if(!canvas) return;
    const ctx = canvas.getContext("2d"); if(!ctx) return;

    const cw = 18, ch = 18;
    canvas.width  = COLS*cw;
    canvas.height = ROWS*ch;

    let animId: number;
    let lastReveal = 0;

    const draw = (now: number) => {
      const s = stateRef.current;
      s.tick++;

      // advance robot in build mode
      if(mode==="build" && now - lastReveal > 280){
        lastReveal = now;
        const dirs=[[-1,0],[0,1],[1,0],[0,-1]];
        const [dr,dc]=dirs[Math.floor(s.tick/4)%4];
        const nr=Math.max(1,Math.min(ROWS-2,s.robot.r+dr));
        const nc=Math.max(1,Math.min(COLS-2,s.robot.c+dc));
        if(BASE_MAP[nr][nc]!==1){
          s.robot = {r:nr,c:nc,angle:Math.atan2(dr,dc)};
        }
        // reveal around robot
        for(let dr2=-4;dr2<=4;dr2++)
          for(let dc2=-4;dc2<=4;dc2++){
            const rr=s.robot.r+dr2, cc=s.robot.c+dc2;
            if(rr>=0&&rr<ROWS&&cc>=0&&cc<COLS) s.revealed[rr][cc]=true;
          }
      }

      // nav mode: slowly animate robot along path
      if(mode==="nav"){
        const pIdx = Math.floor(s.tick/4)%(s.path.length||1);
        if(s.path[pIdx]) {
          const p=s.path[pIdx];
          const p2=s.path[Math.min(pIdx+1,s.path.length-1)];
          s.robot.angle = Math.atan2(p2.r-p.r, p2.c-p.c);
          s.robot.r=p.r; s.robot.c=p.c;
        }
      }

      ctx.clearRect(0,0,canvas.width,canvas.height);

      // grid background
      ctx.fillStyle="#F8FAFA";
      ctx.fillRect(0,0,canvas.width,canvas.height);

      // cells
      for(let r=0;r<ROWS;r++){
        for(let c=0;c<COLS;c++){
          const revealed=s.revealed[r][c];
          const val=BASE_MAP[r][c];
          if(!revealed){
            ctx.fillStyle=T.mapUnknown;
          } else if(val===1){
            ctx.fillStyle=T.mapWall;
          } else {
            ctx.fillStyle=T.mapFree;
          }
          ctx.fillRect(c*cw+1,r*ch+1,cw-2,ch-2);
        }
      }

      // grid lines
      ctx.strokeStyle="rgba(0,0,0,0.05)";
      ctx.lineWidth=0.5;
      for(let r=0;r<=ROWS;r++){ctx.beginPath();ctx.moveTo(0,r*ch);ctx.lineTo(canvas.width,r*ch);ctx.stroke();}
      for(let c=0;c<=COLS;c++){ctx.beginPath();ctx.moveTo(c*cw,0);ctx.lineTo(c*cw,canvas.height);ctx.stroke();}

      // planned path
      if(mode==="nav" && s.path.length>0){
        ctx.strokeStyle=T.teal;
        ctx.lineWidth=2;
        ctx.setLineDash([4,3]);
        ctx.beginPath();
        s.path.forEach((p,i)=>{
          const x=p.c*cw+cw/2, y=p.r*ch+ch/2;
          i===0?ctx.moveTo(x,y):ctx.lineTo(x,y);
        });
        ctx.stroke();
        ctx.setLineDash([]);
      }

      // scan rays
      const rx=s.robot.c*cw+cw/2, ry=s.robot.r*ch+ch/2;
      if(s.revealed[s.robot.r][s.robot.c]){
        ctx.strokeStyle=`rgba(13,148,136,${0.10+0.04*Math.sin(s.tick*0.12)})`;
        ctx.lineWidth=1;
        for(let a=0;a<360;a+=6){
          const rad=(a*Math.PI)/180;
          ctx.beginPath();
          ctx.moveTo(rx,ry);
          ctx.lineTo(rx+Math.cos(rad)*48,ry+Math.sin(rad)*48);
          ctx.stroke();
        }
      }

      // goal marker
      if(mode==="nav"){
        const gx=s.goal.c*cw+cw/2, gy=s.goal.r*ch+ch/2;
        ctx.fillStyle=T.coral;
        ctx.beginPath();
        ctx.arc(gx,gy,6,0,Math.PI*2);
        ctx.fill();
        ctx.strokeStyle="#fff";
        ctx.lineWidth=2;
        ctx.stroke();
        ctx.fillStyle=T.coral;
        ctx.font="bold 9px Inter,sans-serif";
        ctx.textAlign="center";
        ctx.fillText("GOAL",gx,gy-11);
      }

      // robot
      ctx.save();
      ctx.translate(rx,ry);
      ctx.rotate(s.robot.angle);
      ctx.fillStyle=T.teal;
      ctx.beginPath();
      ctx.arc(0,0,7,0,Math.PI*2);
      ctx.fill();
      ctx.strokeStyle="#fff";
      ctx.lineWidth=2;
      ctx.stroke();
      // direction arrow
      ctx.strokeStyle="#fff";
      ctx.lineWidth=2;
      ctx.beginPath();
      ctx.moveTo(0,0);
      ctx.lineTo(10,0);
      ctx.stroke();
      ctx.restore();

      animId = requestAnimationFrame(draw);
    };

    animId = requestAnimationFrame(draw);
    return ()=>cancelAnimationFrame(animId);
  },[mode]);

  return (
    <div style={{position:"relative",width:"100%",height:"100%",background:"#F0F4F8",overflow:"hidden"}}>
      <canvas ref={canvasRef} style={{width:"100%",height:"100%",objectFit:"contain",display:"block"}}/>
      <div style={{
        position:"absolute",bottom:8,left:8,
        background:"rgba(28,31,36,0.72)",color:"#A8D5D0",
        fontSize:10,fontFamily:"JetBrains Mono,monospace",
        padding:"2px 8px",borderRadius:4,
      }}>
        RViz2 · /map
      </div>
      <div style={{
        position:"absolute",top:8,right:8,
        background:"rgba(13,148,136,0.15)",color:T.teal,
        border:`1px solid ${T.teal}`,
        fontSize:10,fontFamily:"JetBrains Mono,monospace",
        padding:"2px 8px",borderRadius:4,
      }}>
        {COLS}×{ROWS} · 0.05m/px
      </div>
    </div>
  );
}

// ─── Gazebo camera feed ────────────────────────────────────────────
function GazeboFeed() {
  const ref = useRef<HTMLCanvasElement>(null);
  const frame = useRef(0);

  useEffect(()=>{
    const canvas = ref.current; if(!canvas) return;
    const ctx = canvas.getContext("2d"); if(!ctx) return;
    canvas.width=480; canvas.height=320;
    let id: number;

    const draw = ()=>{
      const f = ++frame.current;
      const W=canvas.width, H=canvas.height;

      // ── environment ──
      // ceiling
      const ceil=ctx.createLinearGradient(0,0,0,H*0.15);
      ceil.addColorStop(0,"#C8CDD6"); ceil.addColorStop(1,"#D5D9E0");
      ctx.fillStyle=ceil; ctx.fillRect(0,0,W,H*0.15);

      // back wall
      const wall=ctx.createLinearGradient(0,H*0.15,0,H*0.58);
      wall.addColorStop(0,"#BEC5CE"); wall.addColorStop(1,"#C8CDD6");
      ctx.fillStyle=wall; ctx.fillRect(0,H*0.15,W,H*0.43);

      // floor
      const floor=ctx.createLinearGradient(0,H*0.58,0,H);
      floor.addColorStop(0,"#9DA8B4"); floor.addColorStop(1,"#7D8A96");
      ctx.fillStyle=floor; ctx.fillRect(0,H*0.58,W,H*0.42);

      // wall grid lines (perspective)
      ctx.strokeStyle="rgba(255,255,255,0.06)"; ctx.lineWidth=1;
      for(let i=1;i<6;i++){
        const y=H*0.15+i*(H*0.43/6);
        ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke();
      }
      for(let i=1;i<8;i++){
        const x=W*i/8;
        ctx.beginPath(); ctx.moveTo(x,H*0.15); ctx.lineTo(x,H*0.58); ctx.stroke();
      }

      // floor perspective grid
      const vp={x:W/2,y:H*0.58};
      ctx.strokeStyle="rgba(255,255,255,0.10)"; ctx.lineWidth=1;
      for(let i=-8;i<=8;i++){
        ctx.beginPath(); ctx.moveTo(vp.x,vp.y); ctx.lineTo(vp.x+i*80,H); ctx.stroke();
      }
      for(let d=1;d<=5;d++){
        const y=vp.y+(H-vp.y)*(d/5);
        ctx.beginPath(); ctx.moveTo(0,y); ctx.lineTo(W,y); ctx.stroke();
      }

      // warehouse shelves on back wall
      [[W*0.08,H*0.22,W*0.14,H*0.34],[W*0.78,H*0.22,W*0.14,H*0.34]].forEach(([x,y,w,h])=>{
        ctx.fillStyle="rgba(100,110,120,0.55)"; ctx.fillRect(x,y,w,h);
        ctx.strokeStyle="rgba(60,70,80,0.4)"; ctx.lineWidth=1;
        ctx.strokeRect(x,y,w,h);
        for(let sh=1;sh<4;sh++){ctx.beginPath();ctx.moveTo(x,y+h*sh/4);ctx.lineTo(x+w,y+h*sh/4);ctx.stroke();}
      });

      // ── robot body ──
      const rx=W/2, ry=H*0.68;

      // shadow
      ctx.fillStyle="rgba(0,0,0,0.18)";
      ctx.beginPath(); ctx.ellipse(rx,H*0.96,55,8,0,0,Math.PI*2); ctx.fill();

      // chassis
      ctx.fillStyle="#3A4553";
      ctx.beginPath();
      ctx.roundRect(rx-34,ry-28,68,48,4);
      ctx.fill();

      // top sensor dome
      ctx.fillStyle="#2D3545";
      ctx.beginPath();
      ctx.roundRect(rx-20,ry-44,40,20,3);
      ctx.fill();

      // lidar ring
      ctx.strokeStyle="#0D9488"; ctx.lineWidth=2.5;
      ctx.beginPath(); ctx.arc(rx,ry-36,10,0,Math.PI*2); ctx.stroke();

      // spinning lidar beam
      const angle=(f*0.09)%(Math.PI*2);
      ctx.strokeStyle="rgba(13,148,136,0.85)"; ctx.lineWidth=1.5;
      ctx.beginPath();
      ctx.moveTo(rx,ry-36);
      ctx.lineTo(rx+Math.cos(angle)*30,ry-36+Math.sin(angle)*10);
      ctx.stroke();

      // scan fan
      ctx.strokeStyle=`rgba(13,148,136,${0.07+0.03*Math.sin(f*0.06)})`; ctx.lineWidth=1;
      for(let a=-50;a<=50;a+=4){
        const rad=(a*Math.PI)/180;
        ctx.beginPath();
        ctx.moveTo(rx,ry-36);
        ctx.lineTo(rx+Math.sin(rad)*120, ry-36-Math.cos(rad)*80);
        ctx.stroke();
      }

      // wheels
      ctx.fillStyle="#1E2530";
      [{x:rx-38,y:ry+8},{x:rx+22,y:ry+8}].forEach(w=>{
        ctx.beginPath(); ctx.roundRect(w.x,w.y,16,20,3); ctx.fill();
        ctx.strokeStyle="#0D0F14"; ctx.lineWidth=1; ctx.strokeRect(w.x,w.y,16,20);
      });

      // front camera lens
      ctx.fillStyle="#111620";
      ctx.beginPath(); ctx.arc(rx,ry-14,7,0,Math.PI*2); ctx.fill();
      ctx.fillStyle="rgba(13,148,136,0.6)";
      ctx.beginPath(); ctx.arc(rx,ry-14,3,0,Math.PI*2); ctx.fill();

      // brand stripe
      ctx.fillStyle=T.sage;
      ctx.fillRect(rx-34,ry+12,68,6);

      // ── HUD overlays ──
      ctx.strokeStyle=`rgba(13,148,136,${0.35+0.08*Math.sin(f*0.04)})`; ctx.lineWidth=1;
      ctx.strokeRect(10,10,W-20,H-20);

      // crosshair
      const cx2=W/2, cy2=H*0.40;
      ctx.strokeStyle="rgba(13,148,136,0.5)"; ctx.lineWidth=1;
      [[cx2-24,cy2,cx2+24,cy2],[cx2,cy2-16,cx2,cy2+16]].forEach(([x1,y1,x2,y2])=>{
        ctx.beginPath(); ctx.moveTo(x1,y1); ctx.lineTo(x2,y2); ctx.stroke();
      });
      ctx.strokeRect(cx2-16,cy2-12,32,24);

      // corner brackets
      const bLen=14;
      [[18,18],[W-18,18],[18,H-18],[W-18,H-18]].forEach(([bx,by])=>{
        ctx.strokeStyle="rgba(13,148,136,0.45)"; ctx.lineWidth=1.5;
        const sx=bx>W/2?-1:1, sy=by>H/2?-1:1;
        ctx.beginPath();
        ctx.moveTo(bx,by+sy*bLen); ctx.lineTo(bx,by); ctx.lineTo(bx+sx*bLen,by);
        ctx.stroke();
      });

      // telemetry text
      ctx.font="10px 'JetBrains Mono',monospace";
      ctx.textAlign="left";
      [
        {t:`vel_x: ${(0.45+0.05*Math.sin(f*0.03)).toFixed(2)} m/s`,y:28,x:18},
        {t:`vel_z: ${(0.00+0.02*Math.sin(f*0.07)).toFixed(2)} rad/s`,y:42,x:18},
        {t:`pose: (4.2, 2.8)`,y:H-26,x:18},
      ].forEach(({t,x,y})=>{
        ctx.fillStyle="rgba(0,0,0,0.45)";
        ctx.fillRect(x-2,y-11,ctx.measureText(t).width+6,14);
        ctx.fillStyle="#A8D5D0"; ctx.fillText(t,x,y);
      });

      id = requestAnimationFrame(draw);
    };

    id = requestAnimationFrame(draw);
    return ()=>cancelAnimationFrame(id);
  },[]);

  return (
    <div style={{position:"relative",width:"100%",height:"100%",background:"#9DA8B4",overflow:"hidden"}}>
      <canvas ref={ref} style={{width:"100%",height:"100%",display:"block"}}/>
      <div style={{
        position:"absolute",bottom:8,left:8,
        background:"rgba(28,31,36,0.72)",color:"#A8D5D0",
        fontSize:10,fontFamily:"JetBrains Mono,monospace",
        padding:"2px 8px",borderRadius:4,
      }}>
        Gazebo · /camera/rgb
      </div>
      <div style={{
        position:"absolute",top:8,right:8,
        background:"rgba(217,107,107,0.85)",color:"#fff",
        fontSize:10,fontFamily:"JetBrains Mono,monospace",
        padding:"2px 8px",borderRadius:4,letterSpacing:"0.05em",
      }}>
        ● LIVE · 30 Hz
      </div>
    </div>
  );
}

// ─── Feed panel wrapper ────────────────────────────────────────────
function FeedPanel({label,children,flex=1}:{label:string;children:React.ReactNode;flex?:number}){
  return (
    <div style={{
      flex, display:"flex",flexDirection:"column",
      border:`1px solid ${T.borderMed}`,borderRadius:8,
      overflow:"hidden", minHeight:0,
    }}>
      <div style={{
        padding:"6px 12px",
        background:"#F4F5F7",
        borderBottom:`1px solid ${T.border}`,
        fontSize:10,fontWeight:600,letterSpacing:"0.10em",
        textTransform:"uppercase",color:T.dim,
        fontFamily:"Inter,sans-serif",
        flexShrink:0,
      }}>
        {label}
      </div>
      <div style={{flex:1,minHeight:0,overflow:"hidden"}}>
        {children}
      </div>
    </div>
  );
}

// ─── Logging panel ─────────────────────────────────────────────────
type LogLine = { stamp: string; level: "INFO"|"DEBUG"|"WARN"|"ERROR"; msg: string };
const LEVEL_COLOR: Record<string,string> = {
  INFO:"#2C3340", DEBUG:"#7A8496", WARN:"#B97A10", ERROR:"#C53030"
};

function LogPanel({page}:{page:Page}){
  const [lines, setLines] = useState<LogLine[]>([]);
  const indexRef = useRef(0);
  const scrollRef = useRef<HTMLDivElement>(null);

  useEffect(()=>{
    setLines([]); indexRef.current=0;
    const pool = RAW_LOGS[page];
    const push = ()=>{
      const i=indexRef.current;
      if(i<pool.length){
        setLines(p=>[...p, {stamp:ts(), ...pool[i]}]);
        indexRef.current++;
      }
    };
    push();
    const id=setInterval(push,700);
    return()=>clearInterval(id);
  },[page]);

  useEffect(()=>{
    const el=scrollRef.current;
    if(el) el.scrollTop=el.scrollHeight;
  },[lines]);

  return (
    <div style={{
      flexShrink:0,
      height:"22%",
      background:T.logBg,
      borderTop:`1px solid ${T.borderMed}`,
      display:"flex",flexDirection:"column",
    }}>
      {/* header */}
      <div style={{
        display:"flex",alignItems:"center",
        padding:"5px 16px",
        borderBottom:`1px solid ${T.border}`,
        flexShrink:0,
      }}>
        <span style={{
          fontSize:10,fontWeight:600,letterSpacing:"0.12em",
          textTransform:"uppercase",color:T.dim,
          fontFamily:"Inter,sans-serif",
        }}>
          Logger
        </span>
      </div>
      {/* lines — thin scrollbar, always scrollable */}
      <div
        ref={scrollRef}
        style={{
          flex:1,overflowY:"scroll",padding:"6px 16px",
          scrollbarWidth:"thin",
          scrollbarColor:`${T.borderMed} transparent`,
        }}
      >
        {lines.map((l,i)=>(
          <div key={i} style={{
            display:"flex",gap:10,
            fontSize:11,lineHeight:"18px",
            fontFamily:"JetBrains Mono,monospace",
          }}>
            <span style={{color:T.dimLight,flexShrink:0}}>{l.stamp}</span>
            <span style={{
              color: l.level==="INFO"?T.teal : l.level==="DEBUG"?T.dimLight : l.level==="WARN"?"#B97A10":"#C53030",
              flexShrink:0,minWidth:46,
            }}>{l.level}</span>
            <span style={{color:LEVEL_COLOR[l.level]||T.ink}}>{l.msg}</span>
          </div>
        ))}
      </div>
    </div>
  );
}

// ─── Map dropdown ──────────────────────────────────────────────────
const MAPS=["Office","Warehouse"];

function MapDropdown({value,onChange}:{value:string;onChange:(v:string)=>void}){
  const [open,setOpen]=useState(false);
  const ref=useRef<HTMLDivElement>(null);

  useEffect(()=>{
    const handler=(e:MouseEvent)=>{
      if(ref.current && !ref.current.contains(e.target as Node)) setOpen(false);
    };
    document.addEventListener("mousedown",handler);
    return()=>document.removeEventListener("mousedown",handler);
  },[]);

  return (
    <div ref={ref} style={{position:"relative",width:280}}>
      <button
        onClick={()=>setOpen(o=>!o)}
        style={{
          width:"100%",display:"flex",alignItems:"center",justifyContent:"space-between",
          padding:"11px 14px",borderRadius:8,
          background:T.canvas,
          border:`1.5px solid ${open?T.teal:T.borderMed}`,
          color: value?T.ink:T.dim,
          fontSize:14,fontWeight:500,fontFamily:"Inter,sans-serif",
          cursor:"pointer",transition:"border-color 0.15s",
        }}
      >
        <span>{value||"Select Map"}</span>
        <ChevronDown size={16} color={T.teal} strokeWidth={2.4}
          style={{transform:open?"rotate(180deg)":"none",transition:"transform 0.2s"}}/>
      </button>
      {open && (
        <div style={{
          position:"absolute",top:"calc(100% + 4px)",left:0,right:0,
          background:T.canvas,
          border:`1.5px solid ${T.borderMed}`,
          borderRadius:8,overflow:"hidden",
          boxShadow:"0 8px 24px rgba(0,0,0,0.11)",
          zIndex:20,
        }}>
          {MAPS.map((m,i)=>(
            <button
              key={m}
              onClick={()=>{onChange(m);setOpen(false);}}
              className="ros-btn ros-dd-item"
              style={{
                display:"block",width:"100%",textAlign:"left",
                padding:"10px 14px",
                background: value===m?T.tealLight:T.canvas,
                color: value===m?T.teal:T.ink,
                fontSize:13,fontWeight: value===m?600:400,
                fontFamily:"Inter,sans-serif",
                border:"none",cursor:"pointer",
                borderTop: i>0?`1px solid ${T.border}`:"none",
              }}
            >
              {m}
            </button>
          ))}
        </div>
      )}
    </div>
  );
}

// ─── Keyboard guide ────────────────────────────────────────────────
function KeyCap({k,label}:{k:string;label:string}){
  return (
    <div style={{display:"flex",flexDirection:"column",alignItems:"center",gap:4}}>
      <div style={{
        width:38,height:38,display:"flex",alignItems:"center",justifyContent:"center",
        background:T.canvas,
        border:`1.5px solid ${T.borderMed}`,
        borderRadius:7,
        boxShadow:"0 2px 0 rgba(0,0,0,0.10)",
        fontSize:15,fontWeight:700,color:T.ink,
        fontFamily:"JetBrains Mono,monospace",
      }}>{k}</div>
      <span style={{fontSize:10,color:T.dim,fontFamily:"Inter,sans-serif",whiteSpace:"nowrap"}}>{label}</span>
    </div>
  );
}

function KeyboardGuide(){
  return (
    <div style={{
      display:"inline-flex",flexDirection:"column",alignItems:"center",gap:6,
      padding:"14px 24px 12px",
      background:"#EEF1F4",
      border:`1px solid ${T.border}`,
      borderRadius:10,
    }}>
      <span style={{
        fontSize:10,fontWeight:600,letterSpacing:"0.12em",textTransform:"uppercase",
        color:T.dim,fontFamily:"Inter,sans-serif",marginBottom:4,
      }}>
        Teleop Keyboard Controls
      </span>
      <KeyCap k="I" label="Move Forward"/>
      <div style={{display:"flex",gap:8,alignItems:"center"}}>
        <KeyCap k="J" label="Rotate Left"/>
        <KeyCap k="K" label="Stop"/>
        <KeyCap k="L" label="Rotate Right"/>
      </div>
    </div>
  );
}

// ─── Divider ───────────────────────────────────────────────────────
function Divider(){
  return <div style={{height:1,background:T.border,flexShrink:0}}/>;
}

// ─── Status chip ──────────────────────────────────────────────────
function Chip({label,color,bg}:{label:string;color:string;bg:string}){
  return (
    <span style={{
      fontSize:10,fontFamily:"JetBrains Mono,monospace",
      background:bg,color,
      border:`1px solid ${color}33`,
      padding:"2px 9px",borderRadius:4,
      letterSpacing:"0.04em",
    }}>
      {label}
    </span>
  );
}

// ─── Page 1 — Map Selection ────────────────────────────────────────
function P1({onSelect}:{onSelect:(m:string)=>void}){
  const [map,setMap]=useState("");
  return (
    <div className="ros-page" style={{flex:1,display:"flex",flexDirection:"column",alignItems:"center",justifyContent:"center",gap:28}}>
      <div style={{textAlign:"center"}}>
        <h2 style={{fontSize:22,fontWeight:600,color:T.ink,fontFamily:"Inter,sans-serif",marginBottom:6}}>
          On which map do you want to navigate the Bot?
        </h2>
        <p style={{fontSize:13,color:T.dim,fontFamily:"Inter,sans-serif"}}>
          Choose from the pre-loaded maps available on this robot
        </p>
      </div>
      <MapDropdown value={map} onChange={setMap}/>
      {map && (
        <Btn
          label="Confirm Map"
          bg={T.teal}
          onClick={()=>onSelect(map)}
          icon={<ArrowRight size={15}/>}
        />
      )}
    </div>
  );
}

// ─── Page 2 — Mode Selection ───────────────────────────────────────
function P2({mapName,onNav,onAutoMap,onManualMap}:{
  mapName:string;onNav:()=>void;onAutoMap:()=>void;onManualMap:()=>void;
}){
  const [slamOpen,setSlamOpen]=useState(false);
  const ref=useRef<HTMLDivElement>(null);

  useEffect(()=>{
    const h=(e:MouseEvent)=>{
      if(ref.current&&!ref.current.contains(e.target as Node)) setSlamOpen(false);
    };
    document.addEventListener("mousedown",h);
    return()=>document.removeEventListener("mousedown",h);
  },[]);

  return (
    <div className="ros-page" style={{flex:1,display:"flex",flexDirection:"column",alignItems:"center",justifyContent:"center",gap:32}}>
      <div style={{textAlign:"center"}}>
        <h2 style={{fontSize:22,fontWeight:600,color:T.ink,fontFamily:"Inter,sans-serif",marginBottom:6}}>
          Do you want to do Navigation or Mapping?
        </h2>
        <p style={{fontSize:13,color:T.dim,fontFamily:"Inter,sans-serif"}}>
          Active map:&nbsp;
          <span style={{color:T.teal,fontWeight:600,fontFamily:"JetBrains Mono,monospace",fontSize:12}}>
            {mapName}
          </span>
        </p>
      </div>

      <div style={{display:"flex",gap:16,alignItems:"flex-start"}}>
        {/* Nav button */}
        <Btn label="Autonomous Navigation" bg={T.teal} onClick={onNav} wide/>

        {/* SLAM Mapping dropdown */}
        <div ref={ref} style={{position:"relative"}}>
          <button
            onClick={()=>setSlamOpen(o=>!o)}
            style={{
              display:"flex",alignItems:"center",justifyContent:"center",gap:8,
              padding:"13px 36px",borderRadius:7,
              background:T.sage,color:"#fff",
              fontSize:14,fontWeight:600,fontFamily:"Inter,sans-serif",
              border:"none",cursor:"pointer",minWidth:210,
              opacity:1,transition:"opacity 0.15s",letterSpacing:"0.01em",
            }}
            onMouseEnter={e=>(e.currentTarget.style.opacity="0.88")}
            onMouseLeave={e=>(e.currentTarget.style.opacity="1")}
          >
            SLAM Mapping
            <ChevronDown size={15} style={{transform:slamOpen?"rotate(180deg)":"none",transition:"transform 0.2s"}}/>
          </button>
          {slamOpen && (
            <div style={{
              position:"absolute",top:"calc(100% + 4px)",left:0,right:0,
              background:T.canvas,
              border:`1.5px solid ${T.borderMed}`,
              borderRadius:8,overflow:"hidden",
              boxShadow:"0 8px 24px rgba(0,0,0,0.11)",
              zIndex:20,
            }}>
              {[
                {label:"Auto Mapping",sub:"Autonomous frontier exploration",cb:onAutoMap},
                {label:"Manual Mapping",sub:"Teleoperate with keyboard",cb:onManualMap},
              ].map((item,i)=>(
                <button
                  key={item.label}
                  onClick={()=>{setSlamOpen(false);item.cb();}}
                  className="ros-btn ros-dd-item"
                  style={{
                    display:"flex",flexDirection:"column",alignItems:"flex-start",
                    width:"100%",padding:"10px 14px",
                    background:T.canvas,border:"none",cursor:"pointer",
                    borderTop:i>0?`1px solid ${T.border}`:"none",
                  }}
                >
                  <span style={{fontSize:13,fontWeight:600,color:T.ink,fontFamily:"Inter,sans-serif"}}>{item.label}</span>
                  <span style={{fontSize:11,color:T.dim,fontFamily:"Inter,sans-serif"}}>{item.sub}</span>
                </button>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

// ─── Page 3 — Autonomous Navigation ───────────────────────────────
function P3({onBack}:{onBack:()=>void}){
  const [paused,setPaused]=useState(false);
  return (
    <div className="ros-page" style={{flex:1,display:"flex",flexDirection:"column",gap:12,padding:"14px 20px 16px",minHeight:0}}>
      <p style={{textAlign:"center",fontSize:13,color:T.dim,fontFamily:"Inter,sans-serif",flexShrink:0}}>
        Set the destination pose by clicking&nbsp;
        <strong style={{color:T.ink,fontWeight:600}}>2D Nav Goal</strong>
        &nbsp;in the RViz2 panel
      </p>
      <div style={{flex:1,display:"flex",gap:14,minHeight:0}}>
        <FeedPanel label="Map View  —  RViz2 / SLAM">
          <OccupancyMap mode="nav"/>
        </FeedPanel>
        <FeedPanel label="Live Camera  —  Gazebo">
          <GazeboFeed/>
        </FeedPanel>
      </div>
      <div style={{display:"flex",justifyContent:"center",gap:12,flexShrink:0,paddingTop:4}}>
        <Btn
          label={paused?"Resume Navigation":"Pause Navigation"}
          bg={T.blue}
          onClick={()=>setPaused(p=>!p)}
          wide
        />
        <Btn label="Cancel Navigation" bg={T.coral} onClick={onBack} wide/>
      </div>
    </div>
  );
}

// ─── Page 4 — Auto SLAM Mapping ────────────────────────────────────
function P4(){
  return (
    <div className="ros-page" style={{flex:1,display:"flex",flexDirection:"column",gap:12,padding:"14px 20px 16px",minHeight:0}}>
      <p style={{textAlign:"center",fontSize:13,color:T.dim,fontFamily:"Inter,sans-serif",flexShrink:0}}>
        Robot is autonomously exploring and building the map — no operator input required
      </p>
      <div style={{flex:1,display:"flex",gap:14,minHeight:0}}>
        <FeedPanel label="Map View  —  RViz2 / SLAM  (Building)">
          <OccupancyMap mode="build"/>
        </FeedPanel>
        <FeedPanel label="Live Camera  —  Gazebo">
          <GazeboFeed/>
        </FeedPanel>
      </div>
    </div>
  );
}

// ─── Page 5 — Manual SLAM Mapping ─────────────────────────────────
function P5(){
  return (
    <div className="ros-page" style={{flex:1,display:"flex",flexDirection:"column",gap:10,padding:"12px 20px 14px",minHeight:0}}>
      <p style={{textAlign:"center",fontSize:13,color:T.dim,fontFamily:"Inter,sans-serif",flexShrink:0}}>
        Drive the robot manually using keyboard controls to build the map
      </p>
      <div style={{flex:1,display:"flex",gap:14,minHeight:0}}>
        <FeedPanel label="Map View  —  RViz2 / SLAM  (Building)">
          <OccupancyMap mode="build"/>
        </FeedPanel>
        <FeedPanel label="Live Camera  —  Gazebo">
          <GazeboFeed/>
        </FeedPanel>
      </div>
      <div style={{display:"flex",justifyContent:"center",flexShrink:0,paddingTop:2}}>
        <KeyboardGuide/>
      </div>
    </div>
  );
}

// ─── Page metadata ─────────────────────────────────────────────────
const PAGE_META: Record<Page,{title:string;chip?:{label:string;color:string;bg:string}}> = {
  init:      { title:"Map Selection" },
  mode:      { title:"Mode Selection" },
  nav:       { title:"Autonomous Navigation Mode",  chip:{label:"NAV ACTIVE",  color:T.teal,  bg:T.tealLight} },
  automap:   { title:"Auto SLAM Mapping",           chip:{label:"MAPPING",     color:T.sage,  bg:T.sageLight} },
  manualmap: { title:"Manual SLAM Mapping",         chip:{label:"TELEOP",      color:T.blue,  bg:T.blueLight} },
};

// ─── Root ─────────────────────────────────────────────────────────
export default function App(){
  const [page,setPage]=useState<Page>("init");
  const [map,setMap]=useState("");
  const history=useRef<Page[]>([]);

  const go=useCallback((to:Page)=>{ history.current.push(page); setPage(to); },[page]);
  const back=useCallback(()=>{ const p=history.current.pop(); if(p) setPage(p); },[]);
  const canBack=history.current.length>0;

  const meta=PAGE_META[page];
  const pageTitle = page==="mode" && map ? `Map: ${map}` : meta.title;

  return (
    <div style={{
      width:"100%",height:"100%",
      background:T.appBg,
      display:"flex",alignItems:"center",justifyContent:"center",
      fontFamily:"Inter,sans-serif",
    }}>
      <style>{GLOBAL_CSS}</style>
      <div style={{
        width:"min(1060px,97vw)",
        height:"min(680px,97vh)",
        background:T.canvas,
        borderRadius:12,
        boxShadow:"0 2px 8px rgba(0,0,0,0.06),0 8px 40px rgba(0,0,0,0.10)",
        display:"flex",flexDirection:"column",
        overflow:"hidden",
      }}>

        {/* ── Top bar ── */}
        <div style={{
          display:"flex",alignItems:"center",
          padding:"10px 16px",
          borderBottom:`1px solid ${T.border}`,
          flexShrink:0,
          gap:12,
        }}>
          {/* left slot */}
          <div style={{width:110}}>
            {canBack && <BackButton onClick={back}/>}
          </div>

          {/* center */}
          <div style={{flex:1,textAlign:"center"}}>
            <span style={{
              fontSize:12,fontWeight:600,letterSpacing:"0.14em",
              textTransform:"uppercase",color:T.dim,
              fontFamily:"Inter,sans-serif",
            }}>
              {pageTitle}
            </span>
          </div>

          {/* right slot */}
          <div style={{width:110,display:"flex",justifyContent:"flex-end",gap:6}}>
            {meta.chip && <Chip {...meta.chip}/>}
            <Chip label="ROS 2 Jazzy" color={T.teal} bg={T.tealLight}/>
          </div>
        </div>

        <Divider/>

        {/* ── Content ── */}
        <div style={{flex:1,display:"flex",flexDirection:"column",minHeight:0}}>
          {page==="init"      && <P1 onSelect={m=>{setMap(m);go("mode");}}/>}
          {page==="mode"      && <P2 mapName={map} onNav={()=>go("nav")} onAutoMap={()=>go("automap")} onManualMap={()=>go("manualmap")}/>}
          {page==="nav"       && <P3 onBack={back}/>}
          {page==="automap"   && <P4/>}
          {page==="manualmap" && <P5/>}
        </div>

        {/* ── Log panel ── */}
        <LogPanel page={page}/>
      </div>
    </div>
  );
}
