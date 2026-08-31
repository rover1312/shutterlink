// ============================================================================
// web_assets.h — Embedded Glassmorphism Web UI (PROGMEM)
// ============================================================================
// Single-page app served at http://192.168.4.1 when connected to the
// ShutterLink Wi-Fi network.
//
//   • Glassmorphism: frosted cards, backdrop blur, animated gradient blobs
//   • Dark / light theme toggle (persisted in localStorage)
//   • Inline SVG "glass" icon set (stroke + translucent fill, no assets)
//   • Tabs: Dashboard / Controls / Camera / OSD / Flight Controller
//   • Live 1 s polling of /api/status
// ============================================================================

#ifndef WEB_ASSETS_H
#define WEB_ASSETS_H

#include <pgmspace.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en" data-theme="light">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>ShutterLink</title>
<style>
:root{
  --font:'Segoe UI',system-ui,-apple-system,sans-serif;
  --mono:'Cascadia Code',ui-monospace,Consolas,monospace;
  --r:22px;--tr:.35s cubic-bezier(.4,.15,.2,1);
}
[data-theme=dark]{
  --bg:#0a0e17;--txt:#eef2ff;--dim:#9aa7c7;
  --glass:rgba(255,255,255,.055);--glass2:rgba(255,255,255,.09);
  --stroke:rgba(255,255,255,.14);--hi:rgba(255,255,255,.25);
  --accent:#6ea8ff;--rec:#ff4d67;--ok:#3ddc97;--warn:#ffb454;
  --shadow:0 18px 50px rgba(0,0,0,.5);
}
[data-theme=light]{
  --bg:#dce4f2;--txt:#131a2b;--dim:#5b687f;
  --glass:rgba(255,255,255,.55);--glass2:rgba(255,255,255,.72);
  --stroke:rgba(255,255,255,.65);--hi:rgba(255,255,255,.95);
  --accent:#2f6fdb;--rec:#e02e48;--ok:#149e64;--warn:#c47b16;
  --shadow:0 18px 40px rgba(30,45,80,.18);
}
*{box-sizing:border-box;margin:0;-webkit-tap-highlight-color:transparent}
body{font-family:var(--font);background:var(--bg);color:var(--txt);
  min-height:100vh;padding:14px 14px 90px;transition:background .5s,color .5s}
.bg{position:fixed;inset:0;z-index:-1;overflow:hidden}
.blob{position:absolute;border-radius:50%;filter:blur(70px);opacity:.5;animation:drift 26s ease-in-out infinite alternate}
[data-theme=light] .blob{opacity:.55}
.b1{width:46vmax;height:46vmax;left:-14vmax;top:-16vmax;background:radial-gradient(circle,#274bcc,transparent 65%)}
.b2{width:40vmax;height:40vmax;right:-12vmax;top:24vh;background:radial-gradient(circle,#8a2be2,transparent 65%);animation-delay:-9s}
.b3{width:36vmax;height:36vmax;left:22vw;bottom:-18vmax;background:radial-gradient(circle,#0e8a72,transparent 65%);animation-delay:-17s}
@keyframes drift{to{transform:translate(9vmax,6vmax) scale(1.15)}}
.glass{background:var(--glass);border:1px solid var(--stroke);
  -webkit-backdrop-filter:blur(20px) saturate(160%);backdrop-filter:blur(20px) saturate(160%);
  border-radius:var(--r);box-shadow:var(--shadow),inset 0 1px 0 var(--hi)}
header.top{display:flex;align-items:center;gap:12px;padding:14px 18px;margin-bottom:12px}
.logo{display:flex;align-items:center;gap:11px;flex:1;min-width:0}
.logo h1{font-size:19px;font-weight:700;letter-spacing:.4px;white-space:nowrap}
.logo .sub{display:block;font-size:10.5px;font-weight:500;color:var(--dim);letter-spacing:1.6px;text-transform:uppercase}
.pill{display:flex;align-items:center;gap:7px;font-size:12px;font-weight:600;
  padding:7px 13px;border-radius:999px;background:var(--glass2);border:1px solid var(--stroke);white-space:nowrap}
.dot{width:9px;height:9px;border-radius:50%;background:var(--warn);transition:var(--tr)}
.iconbtn{width:42px;height:42px;display:grid;place-items:center;border-radius:14px;cursor:pointer;
  background:var(--glass2);border:1px solid var(--stroke);color:var(--txt);transition:var(--tr)}
.iconbtn:hover{transform:translateY(-2px);border-color:var(--hi)}
nav.tabs{display:flex;gap:6px;padding:7px;margin-bottom:14px;overflow-x:auto;scrollbar-width:none}
nav.tabs::-webkit-scrollbar{display:none}
.tabbtn{flex:1;display:flex;align-items:center;justify-content:center;gap:7px;padding:11px 10px;
  font-size:12.5px;font-weight:600;color:var(--dim);border:none;border-radius:15px;background:transparent;cursor:pointer;
  transition:var(--tr);white-space:nowrap;min-width:86px}
.tabbtn.on{color:var(--txt);background:var(--glass2);box-shadow:inset 0 1px 0 var(--hi)}
main{max-width:760px;margin:0 auto}
.card{padding:20px;margin-bottom:14px;animation:rise .45s ease both}
@keyframes rise{from{opacity:0;transform:translateY(14px)}}
.card h2{display:flex;align-items:center;gap:9px;font-size:13px;font-weight:700;letter-spacing:1.4px;
  text-transform:uppercase;color:var(--dim);margin-bottom:16px}
.grid2{display:grid;grid-template-columns:repeat(auto-fit,minmax(215px,1fr));gap:14px}
.kpi{padding:17px 18px}
.kpi .lbl{font-size:11px;font-weight:600;letter-spacing:1.2px;color:var(--dim);text-transform:uppercase;margin-bottom:9px;display:flex;align-items:center;gap:7px}
.kpi .val{font-size:23px;font-weight:700;font-family:var(--mono)}
.kpi .hint{font-size:11.5px;color:var(--dim);margin-top:5px}
.chip{display:inline-flex;align-items:center;gap:6px;font-size:11.5px;font-weight:700;padding:5px 11px;border-radius:999px;border:1px solid var(--stroke);background:var(--glass2)}
.chip.ok{color:var(--ok)}.chip.rec{color:var(--rec)}.chip.warn{color:var(--warn)}
.hero{text-align:center;padding:26px 20px 22px;position:relative}
.statetime{font-size:44px;font-weight:800;font-family:var(--mono);letter-spacing:1px;margin:10px 0 2px;transition:var(--tr)}
.statecap{font-size:12px;font-weight:700;letter-spacing:2.5px;text-transform:uppercase;color:var(--dim);margin-bottom:22px}
.overlay{position:absolute;inset:0;border-radius:var(--r);display:flex;flex-direction:column;align-items:center;justify-content:center;
  gap:7px;background:var(--glass);border:1px solid var(--stroke);
  -webkit-backdrop-filter:blur(16px) saturate(140%);backdrop-filter:blur(16px) saturate(140%);
  transition:opacity var(--tr);z-index:2;padding:20px;text-align:center}
.overlay.hidden{opacity:0;pointer-events:none}
.overlay b{font-size:15.5px}
.overlay p{font-size:12px;color:var(--dim);max-width:320px;line-height:1.5}
.overlay .ic{width:34px;height:34px;color:var(--dim)}
.rbtns{display:flex;gap:14px;justify-content:center}
.rbtn{width:112px;padding:15px 0;border-radius:19px;border:1px solid var(--stroke);cursor:pointer;
  font-size:13px;font-weight:800;letter-spacing:1.6px;color:var(--txt);background:var(--glass2);
  display:flex;flex-direction:column;align-items:center;gap:8px;transition:var(--tr)}
.rbtn:hover{transform:translateY(-3px)}
.rbtn:active{transform:scale(.96)}
.rbtn.start{border-color:rgba(61,220,151,.4)}
.rbtn.stop{border-color:rgba(255,77,103,.4)}
.osdprev{margin-top:22px;padding-top:16px;border-top:1px dashed var(--stroke);display:flex;flex-direction:column;gap:6px}
.osdprev .l{font-family:var(--mono);font-size:13px;color:var(--dim);display:flex;gap:10px;align-items:baseline}
.osdprev .t{color:var(--txt);min-height:16px}
.field{margin-bottom:17px}
.field label{display:flex;justify-content:space-between;font-size:13px;font-weight:600;margin-bottom:9px}
.field label span{font-family:var(--mono);color:var(--accent)}
select,input[type=text],input[type=password]{width:100%;padding:12px 13px;font-size:14px;font-family:inherit;
  color:var(--txt);background:var(--glass2);border:1px solid var(--stroke);border-radius:14px;outline:none;transition:var(--tr)}
select:focus,input:focus{border-color:var(--accent)}
select option{color:#131a2b;background:#fff}
input[type=range]{width:100%;height:30px;-webkit-appearance:none;background:transparent;cursor:pointer}
input[type=range]::-webkit-slider-runnable-track{height:7px;border-radius:99px;background:linear-gradient(90deg,var(--accent) var(--p,50%),var(--glass2) var(--p,50%));border:1px solid var(--stroke)}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:21px;height:21px;margin-top:-8px;border-radius:50%;
  background:var(--txt);border:2px solid var(--accent);box-shadow:0 2px 8px rgba(0,0,0,.35)}
input[type=range]::-moz-range-track{height:7px;border-radius:99px;background:var(--glass2)}
input[type=range]::-moz-range-thumb{width:19px;height:19px;border-radius:50%;background:var(--txt);border:2px solid var(--accent)}
.switchrow{display:flex;align-items:center;gap:13px;padding:14px 0;border-bottom:1px dashed var(--stroke)}
.switchrow:last-of-type{border-bottom:none}
.sw{position:relative;width:52px;height:29px;flex-shrink:0;cursor:pointer}
.sw input{opacity:0;width:0;height:0}
.sw i{position:absolute;inset:0;border-radius:99px;background:var(--glass2);border:1px solid var(--stroke);transition:var(--tr)}
.sw i:before{content:'';position:absolute;width:21px;height:21px;left:3px;top:3px;border-radius:50%;background:var(--dim);transition:var(--tr)}
.sw input:checked+i{background:linear-gradient(135deg,var(--accent),#8a5cf6);border-color:transparent}
.sw input:checked+i:before{transform:translateX(23px);background:#fff}
.swinfo{flex:1}
.swinfo b{display:block;font-size:14px;margin-bottom:2px}
.swinfo p{font-size:12px;color:var(--dim)}
.btn{padding:12px 22px;border-radius:14px;border:1px solid var(--stroke);cursor:pointer;font-size:13px;
  font-weight:700;color:var(--txt);background:var(--glass2);transition:var(--tr)}
.btn:hover{transform:translateY(-2px);border-color:var(--hi)}
.btn.primary{background:linear-gradient(135deg,var(--accent),#8a5cf6);border-color:transparent;color:#fff}
.btn.danger{border-color:rgba(255,77,103,.45)}
.seg{display:flex;gap:8px}
.seg button{flex:1;display:flex;align-items:center;justify-content:center;gap:8px;padding:13px;border-radius:15px;
  border:1px solid var(--stroke);background:var(--glass2);color:var(--dim);font-weight:700;font-size:13.5px;cursor:pointer;transition:var(--tr)}
.seg button.on{color:var(--txt);border-color:var(--accent);box-shadow:inset 0 0 0 1px var(--accent)}
.note{font-size:12.5px;line-height:1.55;color:var(--dim);padding:13px 15px;border-radius:14px;
  background:var(--glass2);border:1px solid var(--stroke);margin-top:13px}
.slotrow{display:flex;align-items:center;gap:12px;padding:12px 0;border-bottom:1px dashed var(--stroke);flex-wrap:wrap}
.slotrow:last-child{border-bottom:none}
.slotrow .nm{width:158px;font-size:13px;font-weight:700;display:flex;align-items:center;gap:8px}
.slotrow select{flex:1;min-width:140px;width:auto}
.slotrow .pv{width:100%;font-family:var(--mono);font-size:12.5px;color:var(--dim);padding-left:2px}
.camrow{display:flex;align-items:center;gap:12px;padding:13px 14px;border-radius:15px;background:var(--glass2);border:1px solid var(--stroke);margin-bottom:9px}
.camrow .ci{flex:1;min-width:0}
.camrow .ci b{display:block;font-size:14px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.camrow .ci span{font-size:11.5px;color:var(--dim);font-family:var(--mono)}
.camrow .chip{flex-shrink:0}
.btn.mini{padding:8px 15px;font-size:12px;border-radius:11px}
.xbtn{border:none;background:transparent;color:var(--dim);font-size:17px;line-height:1;cursor:pointer;padding:5px 7px;border-radius:9px;transition:var(--tr)}
.xbtn:hover{color:var(--rec);background:var(--glass2)}
.empty{font-size:12.5px;color:var(--dim);text-align:center;padding:20px 10px}
.ic{width:19px;height:19px;flex-shrink:0}
.ic.lg{width:30px;height:30px}
.bar{height:8px;border-radius:99px;background:var(--glass2);border:1px solid var(--stroke);overflow:hidden;margin-top:9px}
.bar i{display:block;height:100%;border-radius:99px;background:linear-gradient(90deg,var(--ok),#ffd166);transition:width .8s var(--tr)}
#toast{position:fixed;left:50%;bottom:26px;transform:translateX(-50%) translateY(90px);z-index:9;
  padding:13px 22px;font-size:13.5px;font-weight:600;border-radius:16px;transition:transform .4s var(--tr);
  max-width:88vw;text-align:center}
#toast.show{transform:translateX(-50%) translateY(0)}
footer{text-align:center;color:var(--dim);font-size:11.5px;padding:18px 0 6px}
  /* Scan results table */
  .scan-table{width:100%;border-collapse:collapse;font-size:12.5px}
  .scan-table th,.scan-table td{padding:10px 12px;text-align:left;border-bottom:1px dashed var(--stroke)}
  .scan-table th{font-weight:700;color:var(--dim);font-size:11px;text-transform:uppercase;letter-spacing:1px}
  .scan-table tr:hover{background:var(--glass2)}
  .scan-table .rssi{font-family:var(--mono);font-size:11.5px}
  .scan-table .rssi.good{color:var(--ok)}
  .scan-table .rssi.mid{color:var(--warn)}
  .scan-table .rssi.bad{color:var(--rec)}
  .scan-table .type-badge{display:inline-block;padding:2px 8px;border-radius:999px;font-size:10.5px;font-weight:700}
  .scan-table .type-badge.dji{background:rgba(47,111,219,.2);color:var(--accent);border:1px solid var(--accent)}
  .scan-table .type-badge.gopro{background:rgba(138,90,246,.2);color:#8a5cf6;border:1px solid #8a5cf6}
  .scan-table .saved-badge{display:inline-flex;align-items:center;gap:4px;padding:2px 8px;border-radius:999px;font-size:10.5px;font-weight:700;color:var(--ok);background:rgba(61,220,151,.15);border:1px solid rgba(61,220,151,.3)}
  .scan-table .active-badge{display:inline-flex;align-items:center;gap:4px;padding:2px 8px;border-radius:999px;font-size:10.5px;font-weight:700;color:var(--accent);background:rgba(110,168,255,.15);border:1px solid rgba(110,168,255,.3)}
  .scan-table .btn.use{background:linear-gradient(135deg,var(--accent),#8a5cf6);border-color:transparent;color:#fff}
  .scan-table .btn.unpair{background:transparent;color:var(--rec);border-color:rgba(255,77,103,.45)}
  .scan-table .btn.unpair:hover{background:rgba(255,77,103,.1)}
  /* Context menu */
  </style>
</head>
<body>
<svg width="0" height="0" style="position:absolute">
<defs>
<symbol id="i-aperture" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9.2" fill="none" stroke="currentColor" stroke-width="1.7"/><path d="M12 3.5 L15.8 9.8 M20.9 9.4 L13.4 10.9 M17.8 19.4 L12.9 13.1 M6.4 19 L10.6 12.9 M3.2 10.2 L10.5 11.4 M8 4.9 L11.4 11" fill="none" stroke="currentColor" stroke-width="1.4" opacity=".85"/><circle cx="12" cy="12" r="3.1" fill="currentColor" opacity=".22"/></symbol>
<symbol id="i-rec" viewBox="0 0 24 24"><circle cx="12" cy="12" r="7.5" fill="currentColor" opacity=".25"/><circle cx="12" cy="12" r="7.5" fill="none" stroke="currentColor" stroke-width="1.8"/><circle cx="12" cy="12" r="3.2" fill="currentColor"/></symbol>
<symbol id="i-stop" viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/><rect x="8.6" y="8.6" width="6.8" height="6.8" rx="1.4" fill="currentColor" opacity=".45"/></symbol>
<symbol id="i-wifi" viewBox="0 0 24 24"><path d="M3.5 9.4 C8.4 4.9 15.6 4.9 20.5 9.4 M6.3 12.7 C9.6 9.7 14.4 9.7 17.7 12.7 M9.1 15.9 C10.8 14.4 13.2 14.4 14.9 15.9" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/><circle cx="12" cy="19" r="1.5" fill="currentColor"/></symbol>
<symbol id="i-sun" viewBox="0 0 24 24"><circle cx="12" cy="12" r="4.4" fill="currentColor" opacity=".3"/><circle cx="12" cy="12" r="4.4" fill="none" stroke="currentColor" stroke-width="1.7"/><g stroke="currentColor" stroke-width="1.7" stroke-linecap="round"><path d="M12 2.8v2.4M12 18.8v2.4M2.8 12h2.4M18.8 12h2.4M5.5 5.5l1.7 1.7M16.8 16.8l1.7 1.7M18.5 5.5l-1.7 1.7M7.2 16.8l-1.7 1.7"/></g></symbol>
<symbol id="i-moon" viewBox="0 0 24 24"><path d="M20.2 13.9 A8.4 8.4 0 1 1 10 3.7 A6.8 6.8 0 0 0 20.2 13.9 Z" fill="currentColor" opacity=".28"/><path d="M20.2 13.9 A8.4 8.4 0 1 1 10 3.7 A6.8 6.8 0 0 0 20.2 13.9 Z" fill="none" stroke="currentColor" stroke-width="1.7"/></symbol>
<symbol id="i-sliders" viewBox="0 0 24 24"><g stroke="currentColor" stroke-width="1.7" stroke-linecap="round"><path d="M4 7h16M4 12h16M4 17h16"/></g><circle cx="9" cy="7" r="2.5" fill="currentColor" opacity=".35"/><circle cx="9" cy="7" r="2.5" fill="none" stroke="currentColor" stroke-width="1.7"/><circle cx="15" cy="12" r="2.5" fill="currentColor" opacity=".35"/><circle cx="15" cy="12" r="2.5" fill="none" stroke="currentColor" stroke-width="1.7"/><circle cx="7" cy="17" r="2.5" fill="currentColor" opacity=".35"/><circle cx="7" cy="17" r="2.5" fill="none" stroke="currentColor" stroke-width="1.7"/></symbol>
<symbol id="i-cam" viewBox="0 0 24 24"><rect x="2.8" y="6.4" width="18.4" height="12.4" rx="3.2" fill="currentColor" opacity=".16"/><rect x="2.8" y="6.4" width="18.4" height="12.4" rx="3.2" fill="none" stroke="currentColor" stroke-width="1.7"/><path d="M8.2 6.2 L9.6 3.8 h4.8 l1.4 2.4" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round"/><circle cx="12" cy="12.6" r="3.4" fill="currentColor" opacity=".35"/><circle cx="12" cy="12.6" r="3.4" fill="none" stroke="currentColor" stroke-width="1.7"/></symbol>
<symbol id="i-chip" viewBox="0 0 24 24"><rect x="6" y="6" width="12" height="12" rx="2.2" fill="currentColor" opacity=".16"/><rect x="6" y="6" width="12" height="12" rx="2.2" fill="none" stroke="currentColor" stroke-width="1.7"/><rect x="9.6" y="9.6" width="4.8" height="4.8" rx="1" fill="currentColor" opacity=".5"/><g stroke="currentColor" stroke-width="1.6" stroke-linecap="round"><path d="M9 2.8v3.2M15 2.8v3.2M9 18v3.2M15 18v3.2M2.8 9h3.2M2.8 15h3.2M18 9h3.2M18 15h3.2"/></g></symbol>
<symbol id="i-layers" viewBox="0 0 24 24"><path d="M12 3.4 L21 8.2 L12 13 L3 8.2 Z" fill="currentColor" opacity=".25"/><path d="M12 3.4 L21 8.2 L12 13 L3 8.2 Z" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round"/><path d="M3.4 12.4 L12 17 L20.6 12.4 M3.4 16.6 L12 21.2 L20.6 16.6" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"/></symbol>
<symbol id="i-bt" viewBox="0 0 24 24"><path d="M6.5 7.5 L17 16.5 L11.8 21 V3 L17 7.5 L6.5 16.5" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round"/></symbol>
<symbol id="i-batt" viewBox="0 0 24 24"><rect x="2.6" y="7.4" width="16.6" height="9.2" rx="2.6" fill="currentColor" opacity=".14"/><rect x="2.6" y="7.4" width="16.6" height="9.2" rx="2.6" fill="none" stroke="currentColor" stroke-width="1.7"/><rect x="5.2" y="10" width="8.4" height="4" rx="1.2" fill="currentColor"/><path d="M21.2 10.4v3.2" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/></symbol>
<symbol id="i-info" viewBox="0 0 24 24"><circle cx="12" cy="12" r="9" fill="currentColor" opacity=".14"/><circle cx="12" cy="12" r="9" fill="none" stroke="currentColor" stroke-width="1.7"/><path d="M12 11v5.4" stroke="currentColor" stroke-width="1.9" stroke-linecap="round"/><circle cx="12" cy="7.6" r="1.25" fill="currentColor"/></symbol>
<symbol id="i-power" viewBox="0 0 24 24"><path d="M12 3v8" stroke="currentColor" stroke-width="1.9" stroke-linecap="round"/><path d="M7.2 6.2a7.6 7.6 0 1 0 9.6 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/></symbol>
<symbol id="i-refresh" viewBox="0 0 24 24"><path d="M20 12a8 8 0 1 1-2.5-5.8 M20 3.6v3.8h-3.8" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/></symbol>
</defs>
</svg>
<div class="bg"><div class="blob b1"></div><div class="blob b2"></div><div class="blob b3"></div></div>
<header class="glass top">
  <div class="logo">
    <svg class="ic lg" style="color:var(--accent)"><use href="#i-aperture"/></svg>
    <h1>ShutterLink<span class="sub">Betaflight Cam Control</span></h1>
  </div>
  <div class="pill" id="statePill"><span class="dot" id="stateDot"></span><span id="stateTxt">CONNECTING</span></div>
  <button class="iconbtn" id="themeBtn" title="Toggle theme">
    <svg class="ic" id="themeIco"><use href="#i-moon"/></svg>
  </button>
</header>

<nav class="glass tabs">
  <button class="tabbtn on" data-tab="dash"><svg class="ic"><use href="#i-rec"/></svg>Dashboard</button>
  <button class="tabbtn" data-tab="ctrl"><svg class="ic"><use href="#i-sliders"/></svg>Controls</button>
  <button class="tabbtn" data-tab="cam"><svg class="ic"><use href="#i-cam"/></svg>Camera</button>
  <button class="tabbtn" data-tab="osd"><svg class="ic"><use href="#i-layers"/></svg>OSD</button>
  <button class="tabbtn" data-tab="fc"><svg class="ic"><use href="#i-chip"/></svg>FC / System</button>
</nav>

<main>
<!-- ============================ DASHBOARD ============================ -->
<section id="tab-dash">
  <div class="glass card hero">
    <div class="chip" id="recChip">STANDBY</div>
    <div class="statetime" id="statetime">--:--</div>
    <div class="statecap" id="statecap">camera link down</div>
    <div class="rbtns" id="rbtns" style="display:none">
      <button class="rbtn start" id="btnStart"><svg class="ic lg" style="color:var(--ok)"><use href="#i-rec"/></svg>START</button>
      <button class="rbtn stop" id="btnStop"><svg class="ic lg" style="color:var(--rec)"><use href="#i-stop"/></svg>STOP</button>
    </div>
    <div class="overlay" id="camDown">
      <svg class="ic"><use href="#i-bt"/></svg>
      <b id="cdTitle">No camera connected</b>
      <p id="cdSub">Searching&hellip; will connect automatically.</p>
    </div>
    <div class="osdprev">
      <div class="l"><span>CM1</span><span class="t" id="pv0"></span></div>
      <div class="l"><span>CM2</span><span class="t" id="pv1"></span></div>
      <div class="l"><span>CM3</span><span class="t" id="pv2"></span></div>
      <div class="l"><span>CM4</span><span class="t" id="pv3"></span></div>
    </div>
  </div>
  <div class="grid2">
    <div class="glass kpi">
      <div class="lbl"><svg class="ic"><use href="#i-batt"/></svg>Camera battery</div>
      <div class="val" id="kBatt">--%</div>
      <div class="bar"><i id="kBattBar" style="width:0%"></i></div>
      <div class="hint" id="kCamModel">&nbsp;</div>
    </div>
    <div class="glass kpi">
      <div class="lbl"><svg class="ic"><use href="#i-sliders"/></svg>Record switch</div>
      <div class="val" id="kRc">---- µs</div>
      <div class="chip warn" id="kSw" style="margin-top:8px">IDLE</div>
      <div class="hint" id="kChLbl">AUX channel —</div>
    </div>
    <div class="glass kpi">
      <div class="lbl"><svg class="ic"><use href="#i-power"/></svg>Betaflight</div>
      <div class="val" id="kVbat">--.-V</div>
      <div class="chip" id="kArmed">FC NOLINK</div>
      <div class="hint" id="kCycle">&nbsp;</div>
    </div>
    <div class="glass kpi">
      <div class="lbl"><svg class="ic"><use href="#i-wifi"/></svg>This device</div>
      <div class="val" id="kHeap">--</div>
      <div class="hint" id="kSys">up -- · clients: -</div>
    </div>
  </div>
</section>
<!-- ============================ CONTROLS ============================ -->
<section id="tab-ctrl" hidden>
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-sliders"/></svg>Record switch</h2>
    <div class="field">
      <label>Switch channel (record / stop)</label>
      <select id="selCh"></select>
    </div>
    <div class="field">
      <label>ON threshold<span id="lblThr">1500 µs</span></label>
      <input type="range" id="rngThr" min="1200" max="1800" step="25" value="1500">
    </div>
    <div class="field">
      <label>Debounce<span id="lblDeb">300 ms</span></label>
      <input type="range" id="rngDeb" min="50" max="1000" step="50" value="300">
    </div>
    <button class="btn primary" id="saveCtrl">Save switch settings</button>
  </div>

  <div class="glass card">
    <h2><svg class="ic"><use href="#i-rec"/></svg>Recording behaviour</h2>
    <div class="switchrow">
      <label class="sw"><input type="checkbox" id="swRoa"><i></i></label>
      <div class="swinfo"><b>Record on arm</b>
        <p>Starts recording automatically when Betaflight arms.</p></div>
    </div>
    <div class="switchrow" style="opacity:.55;transition:var(--tr)">
      <label class="sw"><input type="checkbox" id="swSod"><i></i></label>
      <div class="swinfo"><b>Stop on disarm</b>
        <p>Stop the recording when the FC disarms. If off, keep recording until
        the switch goes low or you stop it here.</p></div>
    </div>
    <div style="margin-top:16px"><button class="btn primary" id="saveBeh">Save behaviour</button></div>
    <div class="note">The record switch always overrides: flipping it OFF clears an
      armed-start. Camera commands are absolute start/stop — never toggles — so
      reconnect retries are safe.</div>
  </div>

  <div class="glass card">
    <h2><svg class="ic"><use href="#i-wifi"/></svg>Wi-Fi access point</h2>
    <div class="field">
      <label>Wi-Fi radio switch</label>
      <select id="selWifiCh"></select>
      <div class="note">Assign a spare switch: flip it low in flight and the
        Wi-Fi radio powers down completely (saves battery, BLE camera link keeps
        running). Flip high to bring the hotspot back. The AP always starts ON
        at boot so you can't lock yourself out.</div>
    </div>
    <div class="field"><label>Network name (SSID)<span id="lblSsid"></span></label>
      <input type="text" id="inSsid" maxlength="32" placeholder="ShutterLink"></div>
    <div class="field"><label>Password (8–64 chars, empty = open)</label>
      <input type="password" id="inPass" maxlength="64" placeholder="unchanged"></div>
    <button class="btn primary" id="saveWifi">Save Wi-Fi &amp; restart AP</button>
    <div class="note">Saving restarts the access point — your phone will disconnect.
      Reconnect to the new network name to continue.</div>
  </div>
</section>

<!-- ============================ CAMERA ============================ -->
<section id="tab-cam" hidden>
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-cam"/></svg>Saved cameras</h2>
    <div id="camList"></div>
    <div class="note"><b>Only saved cameras reconnect automatically.</b> New
      cameras are never paired on their own: discover them below, then tap
      <b>Use</b> to select one — only then does pairing/connection happen.
      Selections and the list survive reboots.</div>
  </div>
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-refresh"/></svg>Pair a new camera</h2>
    <div class="seg">
      <button id="selDji"><svg class="ic"><use href="#i-aperture"/></svg>DJI Osmo Action</button>
      <button id="selGp"><svg class="ic"><use href="#i-bt"/></svg>GoPro HERO8+</button>
    </div>
    <div style="margin-top:16px;display:flex;gap:10px;flex-wrap:wrap">
      <button class="btn primary" id="pairBtn" disabled>Start pairing</button>
      <button class="btn danger" id="btnReboot2">Reboot ESP32</button>
    </div>
    <div class="note"><b>How it works:</b> 1) pick the brand &rarr;
      2) press <b>Start pairing</b> &rarr; 3) power the camera on nearby &rarr;
      4) tap <b>Use</b> on it in the list above. Nothing connects until you
      choose it. First GoPro connection also needs one approval tap on the
      camera screen; DJI may show an approve prompt too.</div>
  </div>
</section>

<!-- ============================ OSD ============================ -->
<section id="tab-osd" hidden>
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-layers"/></svg>Custom messages 1–4</h2>
    <div id="slotRows"></div>
    <div style="margin-top:16px"><button class="btn primary" id="saveSlots">Save OSD slots</button></div>
    <div class="note">Each slot feeds one of Betaflight's four <b>Custom Message</b>
      OSD elements over MSP (<code>MSP2_SET_TEXT</code>, max 16 chars). Place them
      anywhere in the Betaflight Configurator OSD tab — position comes from BF,
      content from here. Live text is previewed on the Dashboard.</div>
  </div>
</section>

<!-- ============================ FC / SYSTEM ============================ -->
<section id="tab-fc" hidden>
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-chip"/></svg>Flight controller</h2>
    <div class="grid2">
      <div class="glass kpi"><div class="lbl">Betaflight API</div><div class="val" id="fApi">--</div><div class="hint">MSP protocol version</div></div>
      <div class="glass kpi"><div class="lbl">Firmware</div><div class="val" id="fFw">--</div><div class="hint" id="fBoard">&nbsp;</div></div>
      <div class="glass kpi"><div class="lbl">Battery</div><div class="val" id="fVbat">--.- V</div><div class="hint">via MSP_ANALOG</div></div>
      <div class="glass kpi"><div class="lbl">State</div><div class="val" id="fArm">--</div><div class="hint" id="fCycle">loop -- µs</div></div>
    </div>
  </div>
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-chip"/></svg>MSP console</h2>
    <div class="field">
      <label>MSP command (read-only)</label>
      <select id="mspCmd">
        <option value="1">1 · API version</option>
        <option value="2">2 · FC variant</option>
        <option value="3">3 · FC firmware version</option>
        <option value="4">4 · Board info</option>
        <option value="5">5 · Build info</option>
        <option value="101" selected>101 · Status (modes, cycle time)</option>
        <option value="104">104 · Motor values</option>
        <option value="105">105 · RC channels</option>
        <option value="109">109 · Altitude</option>
        <option value="110">110 · Analog (battery/RSSI)</option>
        <option value="116">116 · Box names</option>
        <option value="119">119 · Box IDs</option>
        <option value="130">130 · Battery state</option>
      </select>
    </div>
    <div style="display:flex;gap:10px;flex-wrap:wrap">
      <button class="btn primary" id="mspSend">Send MSP query</button>
    </div>
    <pre id="mspOut" style="margin-top:14px;padding:14px;border-radius:14px;background:var(--glass2);
      border:1px solid var(--stroke);font-family:var(--mono);font-size:12px;overflow-x:auto;
      white-space:pre;color:var(--txt)">Send a command to talk to your FC over MSP…</pre>
    <div class="note"><b>About configuring Betaflight from this page:</b> this
      console is a real MSP passthrough — your browser talks to the FC through
      the ESP32, exactly like Betaflight Configurator does over serial. A full
      configurator port isn't realistic on an ESP32-C3 (it's a multi-megabyte
      app), but everything here is scriptable JavaScript: read commands are
      enabled via an allowlist today, and safe parameter writes can be added on
      top of the same endpoint later — think "Lua scripts, in your browser".</div>
  </div>
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-power"/></svg>System</h2>
    <div class="grid2">
      <div class="glass kpi"><div class="lbl">Free heap</div><div class="val" id="sHeap">--</div><div class="hint">bytes available</div></div>
      <div class="glass kpi"><div class="lbl">Uptime</div><div class="val" id="sUp">--</div><div class="hint" id="sIp">&nbsp;</div></div>
    </div>
    <div style="margin-top:16px;display:flex;gap:10px;flex-wrap:wrap">
      <button class="btn danger" id="btnReboot">Reboot ESP32</button>
    </div>
  </div>
</section>

<footer>ShutterLink v2.1 · ESP32-C3 · MSP + BLE bridge<br><span id="ftIp"></span></footer>

<div id="toast" class="glass"></div>
<script>
'use strict';
const $=id=>document.getElementById(id);
let S=null;
/* ---------- tabs ---------- */
document.querySelectorAll('.tabbtn').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('.tabbtn').forEach(x=>x.classList.toggle('on',x===b));
  ['dash','ctrl','cam','osd','fc'].forEach(t=>$('tab-'+t).hidden=(t!==b.dataset.tab));
});

/* ---------- build channel select ---------- */
(()=>{const s=$('selCh');
  for(let i=0;i<16;i++){const o=document.createElement('option');
    o.value=i;o.textContent=i<4?('CH'+(i+1)):('CH'+(i+1)+' \u00b7 AUX'+(i-3));
    s.appendChild(o);}})();

/* ---------- build Wi-Fi switch select (255 = disabled) ---------- */
(()=>{const s=$('selWifiCh');
  const off=document.createElement('option');off.value=255;
  off.textContent='Disabled \u00b7 AP always on';s.appendChild(off);
  for(let i=0;i<16;i++){const o=document.createElement('option');
    o.value=i;o.textContent=i<4?('CH'+(i+1)):('CH'+(i+1)+' \u00b7 AUX'+(i-3));
    s.appendChild(o);}})();
$('selWifiCh').onchange=async()=>{
  try{
    const j=await api('/api/settings',{wifiSwitch:+$('selWifiCh').value});
    toast(j.ok?'Wi-Fi switch saved':'Error: '+(j.error||'?'));
  }catch(e){console.error('selWifiCh error:',e);toast('Error: '+e.message);}
};

/* ---------- build OSD slot rows ---------- */
const SLOT_NAMES=['Off','Cam status','Rec time','Battery','Link state','FC battery','Arm state'];
(()=>{
  const host=$('slotRows');
  for(let i=0;i<4;i++){
    const row=document.createElement('div');row.className='slotrow';
    const nm=document.createElement('div');nm.className='nm';
    nm.innerHTML='CM'+(i+1);
    const sel=document.createElement('select');sel.id='sl'+i;
    SLOT_NAMES.forEach((n,v)=>{const o=document.createElement('option');o.value=v;o.textContent=n;sel.appendChild(o);});
    const pv=document.createElement('div');pv.className='pv';pv.id='spv'+i;
    row.appendChild(nm);row.appendChild(sel);row.appendChild(pv);
    host.appendChild(row);
  }})();

/* ---------- sliders ---------- */
function fill(r){r.style.setProperty('--p',((r.value-r.min)/(r.max-r.min)*100)+'%');}
$('rngThr').oninput=e=>{fill(e.target);$('lblThr').textContent=e.target.value+' \u00b5s';};
$('rngDeb').oninput=e=>{fill(e.target);$('lblDeb').textContent=e.target.value+' ms';};
fill($('rngThr'));fill($('rngDeb'));

/* ---------- API helpers ---------- */
async function api(url,obj,method='POST'){
  try{
    const opts={method,headers:{'Content-Type':'application/json'}};
    if(obj)opts.body=JSON.stringify(obj);
    const r=await fetch(url,opts);
    if(!r.ok){
      const txt=await r.text().catch(()=>'');
      return{ok:false,error:'HTTP ' + r.status + ': ' + (txt||r.statusText)};
    }
    return await r.json();
  }catch(e){
    return{ok:false,error:e.name==='TypeError'?'Network error (check connection)':e.message};
  }
}
$('btnStart').onclick=async()=>{
  try{
    const j=await api('/api/command',{cmd:'start'});
    toast(j.ok?'Record start sent':('Error: '+(j.error||'?')));poll();
  }catch(e){console.error('btnStart error:',e);toast('Error: '+e.message);}
};
$('btnStop').onclick=async()=>{
  try{
    const j=await api('/api/command',{cmd:'stop'});
    toast(j.ok?'Record stop sent':('Error: '+(j.error||'?')));poll();
  }catch(e){console.error('btnStop error:',e);toast('Error: '+e.message);}
};

$('saveCtrl').onclick=async()=>{
  try{
    const j=await api('/api/settings',{auxChannel:+$('selCh').value,
      threshold:+$('rngThr').value,debounce:+$('rngDeb').value});
    toast(j.ok?'Switch settings saved':'Error: '+(j.error||'?'));
  }catch(e){console.error('saveCtrl error:',e);toast('Error: '+e.message);}
};
$('saveBeh').onclick=async()=>{
  try{
    const j=await api('/api/settings',{recordOnArm:$('swRoa').checked,
      stopOnDisarm:$('swSod').checked});
    toast(j.ok?'Behaviour saved':'Error: '+(j.error||'?'));
  }catch(e){console.error('saveBeh error:',e);toast('Error: '+e.message);}
};
$('saveSlots').onclick=async()=>{
  try{
    const body={slot0:+$('sl0').value,slot1:+$('sl1').value,slot2:+$('sl2').value,slot3:+$('sl3').value};
    const j=await api('/api/settings',body);
    toast(j.ok?'OSD slots saved':'Error: '+(j.error||'?'));
  }catch(e){console.error('saveSlots error:',e);toast('Error: '+e.message);}
};

/* ---------- saved-camera list ---------- */
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&','<':'<','>':'>','"':'"'}[c]));
function renderCams(){
  try{
  const list=S.cams||[];const host=$('camList');
  if(!list.length){
    host.innerHTML='<div class="empty">No cameras saved yet.<br>'+
      'Pick a brand below and press <b>Start pairing</b> to discover one.</div>';
    return;}
  const camSt=(S.cam&&S.cam.stateName)||'OFF';
  host.innerHTML=list.map((c,i)=>{
    let badge;
    if(c.on)badge='<span class="chip ok">READY</span>';
    else if(c.a)badge='<span class="chip warn">'+esc(camSt==='SCANNING'?'SCANNING':'CONNECTING')+'</span>';
    else badge='<span class="chip" style="color:var(--dim)">OFFLINE</span>';
    const act=c.a?'<span class="chip" style="color:var(--accent)">ACTIVE</span>'
                 :'<button class="btn mini" data-use="'+i+'">Use</button>';
    return '<div class="camrow" data-mac="'+esc(c.m)+'" data-type="'+(c.t?1:0)+'">'+
      '<svg class="ic" style="color:'+(c.t?'var(--accent)':'var(--txt)')+'"><use href="#'+(c.t?'i-bt':'i-aperture')+'"/></svg>'+
      '<div class="ci"><b>'+esc(c.n||c.m)+'</b><span>'+esc(c.m)+' \u00b7 '+(c.t?'GoPro':'DJI Osmo')+'</span></div>'+
      badge+act+
      '<button class="xbtn" data-del="'+i+'" title="Forget camera">&times;</button></div>';
  }).join('');
  }catch(e){console.error('renderCams error:',e);}
}
$('camList').addEventListener('click',async e=>{
  try{
    const u=e.target.closest('[data-use]');
    if(u){const j=await api('/api/camera',{select:+u.dataset.use});
      toast(j.ok?'Selected \u2014 connecting\u2026 approve prompt on camera if shown':'Error: '+(j.error||'?'));poll();return;}
    const d=e.target.closest('[data-del]');
    if(d){const j=await api('/api/camera',{remove:+d.dataset.del});
      toast(j.ok?'Camera forgotten':'Error: '+(j.error||'?'));poll();}
  }catch(e){console.error('camList click error:',e);toast('Error: '+e.message);}
});

/* ---------- two-step pairing: select brand, then confirm ---------- */
let pendingBrand=-1;
function syncPairUI(){
  try{
    $('selDji').classList.toggle('on',pendingBrand===0);
    $('selGp').classList.toggle('on',pendingBrand===1);
    const pb=$('pairBtn');
    pb.disabled=(pendingBrand<0);
    pb.textContent=pendingBrand<0?'Start pairing'
      :('Start pairing: scan for '+(pendingBrand?'GoPro':'DJI Osmo'));
  }catch(e){console.error('syncPairUI error:',e);}
}
$('selDji').onclick=()=>{try{pendingBrand=0;syncPairUI();}catch(e){console.error(e);}};
$('selGp').onclick=()=>{try{pendingBrand=1;syncPairUI();}catch(e){console.error(e);}};
$('pairBtn').onclick=async()=>{
  try{
    if(pendingBrand<0)return;
    const j=await api('/api/settings',{camera:pendingBrand});
    toast(j.ok?('Scanning for '+(pendingBrand?'GoPro':'DJI Osmo')+
      '\u2026 tap Use when it appears'):'Error: '+(j.error||'?'));
    poll();
  }catch(e){console.error('pairBtn error:',e);toast('Error: '+e.message);}
};
$('saveWifi').onclick=async()=>{
  try{
    const ssid=$('inSsid').value.trim(),pass=$('inPass').value.trim();
    if(!ssid)return toast('Enter an SSID first');
    if(pass&&pass.length<8)return toast('Password must be empty or 8+ chars');
    const body={ssid:ssid};if(pass)body.pass=pass;
    const j=await api('/api/settings',body);
    if(j.ok)toast('Wi-Fi saved \u2014 AP restarting, reconnect to "'+ssid+'"');
    else toast('Error: '+(j.error||'?'));
  }catch(e){console.error('saveWifi error:',e);toast('Error: '+e.message);}
};
['btnReboot','btnReboot2'].forEach(id=>$(id).onclick=async()=>{
  try{
    const j=await api('/api/command',{cmd:'reboot'});
    if(j.ok)toast('Rebooting\u2026 reconnect in ~10 s');
  }catch(e){console.error('reboot error:',e);toast('Error: '+e.message);}
});

/* ---------- MSP console ---------- */
$('mspSend').onclick=async()=>{
  try{
    const out=$('mspOut');
    out.textContent='Querying FC\u2026';
    const j=await api('/api/msp',{cmd:+$('mspCmd').value});
    if(!j.ok){out.textContent='Error: '+(j.error||'?');return;}
    let dump='';
    for(let i=0;i<j.payload.length;i+=32){
      const bytes=(j.payload.substr(i,32).match(/../g)||[]).join(' ');
      dump+='$M> cmd '+String(j.cmd).padStart(3)+'  off '+String(i/2).padStart(3,'0')+'  '+bytes+'\n';}
    out.textContent='OK \u2014 '+j.len+' payload bytes\n\n'+dump;
  }catch(e){console.error('mspSend error:',e);toast('Error: '+e.message);}
};

/* ---------- render ---------- */
const ST_COLORS={READY:'var(--ok)',CONNECTING:'var(--warn)',PAIRING:'var(--warn)',
  SCANNING:'var(--warn)',OFF:'var(--dim)'};
function chLabel(i){return i<4?('CH'+(i+1)):('CH'+(i+1)+' AUX'+(i-3));}
function mmss(s){s=Math.max(0,s|0);return String(Math.floor(s/60)).padStart(2,'0')+':'+String(s%60).padStart(2,'0');}

function render(){
  try{
  if(!S)return;
  const c=S.cam||{},f=S.fc||{},r=S.rec||{},sys=S.sys||{};
  const st=c.stateName||'OFF';

  /* header pill */
  $('stateTxt').textContent=r.desired?'RECORDING':st;
  const dot=$('stateDot');
  dot.style.background=r.desired?'var(--rec)':(ST_COLORS[st]||'var(--dim)');
  dot.style.boxShadow=r.desired?'0 0 12px var(--rec)':'none';

  /* hero */
  const chip=$('recChip');
  const tEl=$('statetime');

  /* no-camera overlay: hide the record buttons until the link is READY */
  const ready=(st==='READY');
  const ov=$('camDown');
  ov.classList.toggle('hidden',ready);
  $('rbtns').style.display=ready?'flex':'none';
  if(!ready){
    const msgs={
      SCANNING:['Searching for camera\u2026','Power the camera on and keep it in pairing range. Connecting automatically.'],
      CONNECTING:['Connecting\u2026','Almost there.'],
      PAIRING:['Pairing required','Approve the pairing prompt on the camera screen once \u2014 it will reconnect silently after that.'],
      OFF:['No camera connected','The BLE link is down. Retrying automatically every few seconds.']};
    const m=msgs[st]||msgs.OFF;
    $('cdTitle').textContent=m[0];
    $('cdSub').textContent=m[1];
    tEl.textContent='--:--';
    tEl.style.color='var(--dim)';
    chip.textContent='NO CAMERA';chip.className='chip warn';
    $('statecap').textContent='';
  } else {
    chip.textContent=r.desired?'RECORDING':'STANDBY';
    chip.className='chip '+(r.desired?'rec':'ok');
    tEl.textContent=(c.recTime!=null)?mmss(c.recTime):'--:--';
    tEl.style.color=r.desired?'var(--rec)':'var(--txt)';
    $('statecap').textContent=(c.model||c.name||'camera')+' \u00b7 ready';
  }

  /* OSD previews */
  const osd=S.osd||['','','',''];
  const slots=S.slots||[0,0,0,0];
  for(let i=0;i<4;i++){$('pv'+i).textContent=osd[i]||'\u2014';
    $('spv'+i).textContent='now: '+(osd[i]||'(blank)');
    const sel=$('sl'+i);
    if(document.activeElement!==sel)sel.value=slots[i];}

  /* KPI cards */
  const b=c.batt;
  $('kBatt').textContent=(b!=null&&b>=0)?b+'%':'--%';
  $('kBattBar').style.width=(b!=null&&b>=0)?b+'%':'0%';
  $('kCamModel').textContent=c.model||('\u00a0');
  $('kRc').textContent=(r.rcValue>0)?(r.rcValue+' \u00b5s'):'---- \u00b5s';
  const sw=$('kSw');
  if(r.rcValue>0){sw.textContent=r.switchOn?'SWITCH ON':'SWITCH OFF';
    sw.className='chip '+(r.switchOn?'rec':'ok');}
  else{sw.textContent='NO RC DATA';sw.className='chip warn';}
  $('kChLbl').textContent='switch on '+chLabel(r.auxCh!=null?r.auxCh:0);

  $('kVbat').textContent=f.alive?(f.vbat10/10).toFixed(1)+' V':'--.- V';
  const ar=$('kArmed');
  ar.textContent=!f.alive?'FC NOLINK':(f.armed?'ARMED':'DISARMED');
  ar.className='chip '+(f.armed&&f.alive?'rec':(f.alive?'ok':'warn'));
  $('kCycle').textContent=f.cycle?('loop '+f.cycle+' \u00b5s'):'\u00a0';

  $('kHeap').textContent=sys.heap?Math.round(sys.heap/1024)+'K free':'--';
  $('kSys').textContent='up '+mmss(sys.uptime||0)+' \u00b7 clients: '+(sys.sta??'-');

  /* FC tab */
  $('fApi').textContent=f.api||'--';
  $('fFw').textContent=f.fw||'--';
  $('fBoard').textContent=f.board||'\u00a0';
  $('fVbat').textContent=f.alive?(f.vbat10/10).toFixed(1)+' V':'--.- V';
  $('fArm').textContent=f.alive?(f.armed?'ARMED':'READY'):'NOLINK';
  $('fCycle').textContent=f.cycle?('loop '+f.cycle+' \u00b5s \u00b7 rssi '+f.rssi):'';

  /* System tab */
  $('sHeap').textContent=sys.heap?Math.round(sys.heap/1024)+'K':'--';
  $('sUp').textContent=mmss(sys.uptime||0);
  $('sIp').textContent=sys.ip?('http://'+sys.ip+'/'):'';
  $('ftIp').textContent=sys.ip?('http://'+sys.ip+'/'):'';
  }catch(e){console.error('Render error:',e);}
}

/* ---------- poll ---------- */
async function poll(){
  try{
    const r=await fetch('/api/status',{cache:'no-store'});
    if(!r.ok)throw new Error('HTTP ' + r.status);
    S=await r.json();
    /* sync forms (skip focused elements so typing isn't clobbered) */
    const rec=S.rec||{},ae=document.activeElement;
    if(ae!==$('selCh'))$('selCh').value=(rec.auxCh!=null)?rec.auxCh:8;
    if(ae!==$('rngThr')){$('rngThr').value=rec.thr||1500;fill($('rngThr'));
      $('lblThr').textContent=$('rngThr').value+' \u00b5s';}
    if(ae!==$('rngDeb')){$('rngDeb').value=rec.deb||300;fill($('rngDeb'));
      $('lblDeb').textContent=$('rngDeb').value+' ms';}
    if(ae!==$('swRoa'))$('swRoa').checked=!!S.roa;
    $('swSod').disabled=!S.roa;
    $('swSod').parentElement.parentElement.style.opacity=S.roa?1:.55;
    if(ae!==$('selWifiCh'))$('selWifiCh').value=(S.wifiSwitch!=null&&S.wifiSwitch>=0)?S.wifiSwitch:255;
    /* brand pills: show pending choice while choosing, else the active one */
    if(pendingBrand>=0)syncPairUI();
    else{$('selDji').classList.toggle('on',!!S.cam&&S.cam.type===0);
         $('selGp').classList.toggle('on',!!S.cam&&S.cam.type===1);}
    renderCams();
    render();
  }catch(e){
    console.error('Poll error:',e);
    try{$('stateTxt').textContent='OFFLINE';}catch(_){}
    try{$('stateDot').style.background='var(--warn)';}catch(_){}
  }
}
setInterval(poll,1000);
poll();
</script>
</body>
</html>)rawliteral";

#endif // WEB_ASSETS_H
