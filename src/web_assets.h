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
  /* Active camera card (Card 1) */
  .active-card{display:flex;align-items:center;gap:14px;padding:16px;border-radius:15px;background:var(--glass2);border:1px solid var(--stroke);margin-bottom:10px}
  .active-card .ic-lg{width:38px;height:38px;flex-shrink:0;color:var(--accent)}
  .active-card .ac-info{flex:1;min-width:0}
  .active-card .ac-info b{display:block;font-size:15px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  .active-card .ac-info span{display:block;font-size:11.5px;color:var(--dim);font-family:var(--mono);margin-top:2px}
  .active-card .ac-actions{display:flex;gap:8px;flex-shrink:0}
  /* Discovered (not yet saved) device rows */
  .discrow{display:flex;align-items:center;gap:12px;padding:11px 13px;border-radius:13px;background:var(--glass2);border:1px dashed var(--stroke);margin-bottom:7px}
  .discrow .ci{flex:1;min-width:0}
  .discrow .ci b{display:block;font-size:13.5px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  .discrow .ci span{font-size:11px;color:var(--dim);font-family:var(--mono)}
  .discrow .rssi-mini{font-family:var(--mono);font-size:11px;color:var(--dim);margin-right:6px}
  .discrow .rssi-mini.good{color:var(--ok)}
  .discrow .rssi-mini.mid{color:var(--warn)}
  .discrow .rssi-mini.bad{color:var(--rec)}
  .btn.pair-save{background:linear-gradient(135deg,var(--ok),#2bb583);border-color:transparent;color:#fff;padding:7px 13px;font-size:12px;font-weight:700;border-radius:10px;cursor:pointer}
  .btn.pair-save:hover{transform:translateY(-2px)}
  .btn.pair-save:disabled{opacity:.5;cursor:not-allowed;transform:none}
  .btn.disconnect{background:transparent;border:1px solid rgba(255,77,103,.45);color:var(--rec);padding:10px 18px;font-size:13px;font-weight:700;border-radius:13px;cursor:pointer}
  .btn.disconnect:hover{background:rgba(255,77,103,.1)}
  .spinner-dot{display:inline-block;width:12px;height:12px;border:2px solid var(--stroke);border-top-color:var(--warn);border-radius:50%;animation:spin 1s linear infinite}
  @keyframes spin{to{transform:rotate(360deg)}}
  /* "Show all nearby devices" link-like toggle */
  .link-row{margin-top:10px;text-align:right}
  a.linklike{color:var(--accent);font-size:12.5px;text-decoration:underline;cursor:pointer;opacity:.85}
  a.linklike:hover{opacity:1}
  a.linklike.on{color:var(--rec);font-weight:600}
  /* RSSI signal-bar (4 bars, top-up) */
  .rssi-bar{display:inline-flex;align-items:flex-end;gap:1.5px;height:14px;margin-right:4px;vertical-align:middle}
  .rssi-bar i{display:inline-block;width:3px;background:var(--dim);border-radius:1px;opacity:.35}
  .rssi-bar i:nth-child(1){height:4px}
  .rssi-bar i:nth-child(2){height:7px}
  .rssi-bar i:nth-child(3){height:10px}
  .rssi-bar i:nth-child(4){height:14px}
  .rssi-bar.s1 i:nth-child(-n+1){background:var(--rec);opacity:1}
  .rssi-bar.s2 i:nth-child(-n+2){background:var(--rec);opacity:1}
  .rssi-bar.s3 i:nth-child(-n+3){background:var(--warn);opacity:1}
  .rssi-bar.s4 i{background:var(--ok);opacity:1}
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
  <!-- =================== CARD 1: ACTIVE CONNECTION =================== -->
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-bt"/></svg>Active connection</h2>
    <div id="activeCam"></div>
  </div>

  <!-- =================== CARD 2: SAVED CAMERAS =================== -->
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-cam"/></svg>Saved cameras</h2>
    <div id="camList"></div>
  </div>

  <!-- =================== CARD 3: DISCOVER NEW CAMERA =================== -->
  <div class="glass card">
    <h2><svg class="ic"><use href="#i-refresh"/></svg>Discover new camera</h2>
    <div class="seg" id="discoverSeg">
      <button id="selDji" data-brand="0"><svg class="ic"><use href="#i-aperture"/></svg>DJI Osmo Action</button>
      <button id="selGp" data-brand="1"><svg class="ic"><use href="#i-bt"/></svg>GoPro HERO8+</button>
    </div>
    <div style="margin-top:14px;display:flex;gap:10px;flex-wrap:wrap;align-items:center">
      <button class="btn primary" id="scanBtn" disabled>
        <svg class="ic" style="vertical-align:middle" id="scanIcon"><use href="#i-refresh"/></svg>
        <span id="scanBtnTxt">Scan for Cameras</span>
      </button>
      <span id="scanCountdown" style="font-size:12px;color:var(--dim)"></span>
    </div>
    <div id="scanSpinner" style="display:none;margin-top:10px;align-items:center;gap:8px;color:var(--warn);font-size:12.5px">
      <span class="spinner-dot"></span><span>Scanning…</span>
    </div>
    <div class="link-row">
      <a href="#" id="scanAllToggle" class="linklike">Camera not listed? Show all nearby devices</a>
    </div>
    <div id="scanAllHint" class="note" style="display:none">
      <b>Showing every advertiser with a valid address.</b> Hold the camera
      within 1&nbsp;m — the strongest signal is usually yours. Pick your
      device and tap <b>Pair &amp; Save</b>; if the connection fails with
      &ldquo;not a &lt;brand&gt; camera&rdquo; you picked the wrong one.
    </div>
    <div id="discList" style="margin-top:14px"></div>
    <div class="note"><b>How pairing works:</b> 1) pick the brand &rarr; 2) press
      <b>Scan for Cameras</b> &rarr; 3) power the camera on nearby &rarr; 4) tap
      <b>Pair &amp; Save</b> on the device. Nothing connects until you confirm.
      First GoPro connection needs one approval tap on the camera screen; DJI
      may show an approve prompt too.</div>
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
/* ---------- Toast polyfill (Captive Portal has no internet; CDN libs unavailable) ---------- */
if (typeof toast !== 'function') {
    window.toast = function(msg, type="info") {
        console.log("[Toast "+type+"]", msg);
        let el = document.createElement('div');
        el.innerText = msg;
        el.style.cssText = "position:fixed;bottom:20px;right:20px;background:rgba(15,23,42,0.9);color:white;padding:12px 24px;border-radius:12px;z-index:9999;font-family:sans-serif;box-shadow:0 8px 16px rgba(0,0,0,0.3);backdrop-filter:blur(8px);border:1px solid rgba(255,255,255,0.1);transition:opacity 0.4s, transform 0.4s;transform:translateY(20px);opacity:0;";
        document.body.appendChild(el);
        setTimeout(function() { el.style.opacity = '1'; el.style.transform = 'translateY(0)'; }, 10);
        setTimeout(function() { el.style.opacity = '0'; el.style.transform = 'translateY(20px)'; setTimeout(function() { el.remove(); }, 400); }, 3000);
    };
}
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

/* ---------- camera tab: 3 cards (active / saved / discover) ---------- */
const esc=s=>String(s).replace(/[&<>"]/g,c=>({'&':'&','<':'<','>':'>','"':'"'}[c]));
let lastErrorShown='';  // last backend lastError string already toasted

function renderActiveCam(){
  const host=$('activeCam');
  const c=S.cam||{};
  const list=S.cams||[];
  const st=(c.stateName||'OFF');
  // Find the active saved camera (if any)
  const active=list.find(x=>x.a);

  if(!active){
    host.innerHTML='<div class="empty">No camera connected.<br>Select one from your saved devices below, or scan for a new one.</div>';
    return;
  }

  let badge;
  if(st==='READY')badge='<span class="chip ok">READY</span>';
  else if(st==='SCANNING'||st==='CONNECTING'||st==='PAIRING')
    badge='<span class="chip warn">'+esc(st)+'</span>';
  else badge='<span class="chip" style="color:var(--dim)">'+esc(st)+'</span>';

  const ico=active.t?'i-bt':'i-aperture';
  const brandName=active.t?'GoPro':'DJI Osmo';

  host.innerHTML=
    '<div class="active-card">'+
      '<svg class="ic-lg"><use href="#'+ico+'"/></svg>'+
      '<div class="ac-info">'+
        '<b>'+esc(active.n||active.m)+'</b>'+
        '<span>'+esc(active.m)+' \u00b7 '+brandName+'</span>'+
      '</div>'+
      '<div class="ac-actions">'+badge+
        '<button class="btn disconnect" data-disconnect="'+list.indexOf(active)+'">Disconnect</button>'+
      '</div>'+
    '</div>';
}

function renderCams(){
  try{
  const list=S.cams||[];const host=$('camList');
  if(!list.length){
    host.innerHTML='<div class="empty">No saved cameras. Scan for a new device below.</div>';
    return;}
  host.innerHTML=list.map((c,i)=>{
    const badge=!c.a?'<span class="chip" style="color:var(--dim)">SAVED</span>':'';
    const act=c.a?'<span class="chip" style="color:var(--accent)">ACTIVE</span>'
                 :'<button class="btn mini" data-connect="'+i+'">Connect</button>';
    return '<div class="camrow" data-mac="'+esc(c.m)+'" data-type="'+(c.t?1:0)+'">'+
      '<svg class="ic" style="color:'+(c.t?'var(--accent)':'var(--txt)')+'"><use href="#'+(c.t?'i-bt':'i-aperture')+'"/></svg>'+
      '<div class="ci"><b>'+esc(c.n||c.m)+'</b><span>'+esc(c.m)+' \u00b7 '+(c.t?'GoPro':'DJI Osmo')+'</span></div>'+
      badge+act+
      '<button class="xbtn" data-forget="'+i+'" title="Forget camera">&times;</button></div>';
  }).join('');
  }catch(e){console.error('renderCams error:',e);}
}

$('camList').addEventListener('click',async e=>{
  try{
    const c=e.target.closest('[data-connect]');
    if(c){const j=await api('/api/camera',{select:+c.dataset.connect});
      toast(j.ok?'Selected \u2014 connecting\u2026 approve prompt on camera if shown':'Error: '+(j.error||'?'));poll();return;}
    const f=e.target.closest('[data-forget]');
    if(f){if(!confirm('Forget this camera? It will need to be re-paired.'))return;
      const j=await api('/api/camera',{remove:+f.dataset.forget});
      toast(j.ok?'Camera forgotten':'Error: '+(j.error||'?'));poll();return;}
    const d=e.target.closest('[data-disconnect]');
    if(d){
      // Disconnect: remove the active camera from the saved list so the loop
      // doesn't auto-reconnect. The user can re-pair later.
      if(!confirm('Disconnect and forget this camera?'))return;
      const j=await api('/api/camera',{remove:+d.dataset.disconnect});
      toast(j.ok?'Disconnected':'Error: '+(j.error||'?'));poll();return;}
  }catch(e){console.error('camList click error:',e);toast('Error: '+e.message);}
});

/* ---------- discover (Card 3): brand pills + scan button + cooldown ---------- */
let pendingBrand=-1;         // -1 = none, 0 = DJI, 1 = GoPro
let scanCooldownUntil=0;     // ms timestamp when the scan button re-enables
let userScanActive=false;    // true ONLY between user click and cooldown end
let discoveredCams=[];       // last seen discovered list from /api/scan
let scanResultsActive=false; // mirrors S.scanning for poller use
let scanPollTimer=null;      // setInterval for /api/scan polling
let scanCdTimer=null;        // setInterval that ticks the countdown UI

function syncDiscoverUI(){
  try{
    $('selDji').classList.toggle('on',pendingBrand===0);
    $('selGp').classList.toggle('on',pendingBrand===1);
    const btn=$('scanBtn');
    const txt=$('scanBtnTxt');
    const sp=$('scanSpinner');
    const now=Date.now();
    const cdLeft=Math.max(0,Math.ceil((scanCooldownUntil-now)/1000));

    // Three explicit UI states — driven ONLY by user action + cooldown,
    // never by the backend's auto-scan state. This prevents the button
    // from being stuck on "Scanning…" just because cam_manager is doing
    // its own background re-scan loop.
    if(userScanActive && cdLeft>0){
      // Cooldown is in progress: disable, show countdown, hide spinner.
      btn.disabled=true;
      txt.textContent='Scan for Cameras (Ready in '+cdLeft+'s)';
      sp.style.display='none';
    }else if(userScanActive && cdLeft<=0){
      // Cooldown elapsed: re-enable for the next user click.
      userScanActive=false;
      btn.disabled=(pendingBrand<0);
      txt.textContent=pendingBrand<0?'Pick a brand first'
        :('Scan for '+(pendingBrand?'GoPro':'DJI Osmo'));
      sp.style.display='none';
    }else if(pendingBrand<0){
      // No brand picked yet: disabled with hint.
      btn.disabled=true;
      txt.textContent='Pick a brand first';
      sp.style.display='none';
    }else{
      // Ready: enabled, label shows what brand will be scanned.
      btn.disabled=false;
      txt.textContent='Scan for '+(pendingBrand?'GoPro':'DJI Osmo');
      sp.style.display='none';
    }
  }catch(e){console.error('syncDiscoverUI error:',e);}
}

function rssiBars(r){
  // 4-bar visual: -50dBm or stronger = 4 bars, -65 = 3, -80 = 2, else 1.
  let n = 1;
  if (r >= -50) n = 4;
  else if (r >= -65) n = 3;
  else if (r >= -80) n = 2;
  return '<span class="rssi-bar s'+n+'" title="'+r+' dBm"><i></i><i></i><i></i><i></i></span>';
}

function renderDiscovered(){
  const host=$('discList');
  if(!discoveredCams||!discoveredCams.length){
    host.innerHTML='<div class="empty">No devices found yet. Press <b>Scan for Cameras</b> above.</div>';
    return;
  }
  // Dedupe by MAC (in case of duplicates)
  const seen=new Set();
  const uniq=discoveredCams.filter(r=>{
    if(seen.has(r.mac))return false;
    seen.add(r.mac);
    return true;
  });
  // Sort: saved last (they already appear in Card 2). The backend already
  // returns results sorted by RSSI desc; preserve that order here.
  const savedMACS=new Set((S.cams||[]).map(c=>c.m));
  const newOnes=uniq.filter(r=>!savedMACS.has(r.mac));
  if(!newOnes.length){
    host.innerHTML='<div class="empty">No new devices. All found cameras are already saved.</div>';
    return;
  }
  host.innerHTML=newOnes.map(r=>{
    const typeLabel=r.t==='GoPro'?'GoPro':'DJI Osmo';
    const typeIcon=r.t==='GoPro'?'i-bt':'i-aperture';
    return '<div class="discrow" data-mac="'+esc(r.mac)+'">'+
      '<svg class="ic" style="color:var(--warn)"><use href="#'+typeIcon+'"/></svg>'+
      '<div class="ci"><b>'+esc(r.n||r.mac)+'</b>'+
        '<span>'+esc(r.mac)+' \u00b7 '+typeLabel+'</span></div>'+
      rssiBars(r.rssi)+
      '<button class="btn pair-save" data-pair="'+esc(r.mac)+'" data-pair-type="'+(r.t==='GoPro'?1:0)+'">Pair &amp; Save</button>'+
    '</div>';
  }).join('');
}

$('discList').addEventListener('click',async e=>{
  try{
    const p=e.target.closest('[data-pair]');
    if(p){
      const mac=p.dataset.pair;
      const type=+p.dataset.pairType;
      p.disabled=true;
      const j=await api('/api/camera',{pair:{mac:mac,type:type}});
      if(j.ok){
        toast('Saved! Connecting\u2026');
        scanCooldownUntil=Date.now()+10000; // 10s cooldown after a pair
      }else{
        toast('Error: '+(j.error||'?'));
        p.disabled=false;
      }
      poll();
    }
  }catch(e){console.error('pair click error:',e);toast('Error: '+e.message);}
});

async function pollDiscovered(){
  try{
    const r=await fetch('/api/scan',{cache:'no-store'});
    if(r.ok){
      const d=await r.json();
      discoveredCams=d.results||[];
      scanResultsActive=!!d.scanning;
      renderDiscovered();
    }
  }catch(e){/* silent — radio contention */}
}
function startScanPoll(){
  if(scanPollTimer)return;
  pollDiscovered();
  scanPollTimer=setInterval(pollDiscovered,1500);
}
function stopScanPoll(){
  if(scanPollTimer){clearInterval(scanPollTimer);scanPollTimer=null;}
}

function startCooldownTicker(){
  if(scanCdTimer)return;
  scanCdTimer=setInterval(()=>{
    syncDiscoverUI();
    const now=Date.now();
    // Once the cooldown finishes and the scan poller has wound down,
    // re-enable the button by clearing userScanActive.
    if(now>scanCooldownUntil && userScanActive){
      // Give the poller ~1s extra to grab the last few results, then stop.
      if(!scanResultsActive){
        userScanActive=false;
        stopScanPoll();
        syncDiscoverUI();
        clearInterval(scanCdTimer);
        scanCdTimer=null;
      }
    }
  },500);
}

$('selDji').onclick=()=>{try{pendingBrand=0;syncDiscoverUI();}catch(e){console.error(e);}};
$('selGp').onclick=()=>{try{pendingBrand=1;syncDiscoverUI();}catch(e){console.error(e);}};

/* ---------- "Show all nearby devices" toggle ---------- */
$('scanAllToggle').onclick=async e=>{
  try{
    e.preventDefault();
    const want = !S.scanAll;
    const j=await api('/api/settings',{scanAll:want});
    if(!j.ok){toast('Error: '+(j.error||'?'));return;}
    S.scanAll = want;
    $('scanAllToggle').classList.toggle('on', want);
    $('scanAllToggle').textContent = want
      ? 'Showing all nearby devices — click to filter to known cameras'
      : 'Camera not listed? Show all nearby devices';
    $('scanAllHint').style.display = want ? 'block' : 'none';
    toast(want ? 'Showing every nearby device' : 'Filtered to known cameras');
    // Trigger a fresh scan so the new mode takes effect immediately.
    if(!userScanActive && pendingBrand>=0){
      $('scanBtn').click();
    } else {
      poll();
    }
  }catch(e){console.error('scanAllToggle error:',e);toast('Error: '+e.message);}
};
// Sync the toggle label on initial load and on every poll.
function syncScanAllToggle(){
  const want = !!S.scanAll;
  const link = $('scanAllToggle');
  if(!link) return;
  link.classList.toggle('on', want);
  link.textContent = want
    ? 'Showing all nearby devices — click to filter to known cameras'
    : 'Camera not listed? Show all nearby devices';
  const hint = $('scanAllHint');
  if(hint) hint.style.display = want ? 'block' : 'none';
}

$('scanBtn').onclick=async()=>{
  try{
    if(pendingBrand<0)return toast('Pick a brand first');
    if(userScanActive)return;  // already in cooldown/scanning
    // 1) Trigger the backend scan (non-blocking; 5s window).
    const j=await api('/api/settings',{camera:pendingBrand});
    if(!j.ok){toast('Error: '+(j.error||'?'));return;}
    // 2) Mark the UI as "user is scanning" + start 10s cooldown.
    userScanActive=true;
    scanCooldownUntil=Date.now()+10000;
    // 3) Show the spinner for the first 5s of the cooldown (scan window).
    $('scanSpinner').style.display='flex';
    setTimeout(()=>{ $('scanSpinner').style.display='none'; }, 5000);
    // 4) Start polling /api/scan so Card 3 fills with results.
    startScanPoll();
    startCooldownTicker();
    syncDiscoverUI();
    toast('Scanning for '+(pendingBrand?'GoPro':'DJI Osmo')+'\u2026');
  }catch(e){console.error('scanBtn error:',e);toast('Error: '+e.message);}
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
['btnReboot'].forEach(id=>{const el=$(id);if(el)el.onclick=async()=>{
  try{
    const j=await api('/api/command',{cmd:'reboot'});
    if(j.ok)toast('Rebooting\u2026 reconnect in ~10 s');
  }catch(e){console.error('reboot error:',e);toast('Error: '+e.message);}
}});

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

/* ---------- poll (silent on radio contention, 1500ms interval) ---------- */
async function poll(){
  try{
    const res = await fetch('/api/status',{cache:'no-store'});
    if(res.ok){
      S = await res.json();
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
      // brand pills reflect the live backend brand only when user has NOT
      // already picked a pending brand.
      if(pendingBrand<0){
        $('selDji').classList.toggle('on',!!S.cam&&S.cam.type===0);
        $('selGp').classList.toggle('on',!!S.cam&&S.cam.type===1);
      }
      renderActiveCam();
      renderCams();
      renderDiscovered();
      syncDiscoverUI();
      syncScanAllToggle();
      render();
      // Surface backend connect-attempt errors as a single toast (only when
      // they change, so we don't spam on every 1.5s poll).
      if (S.lastError && S.lastError !== lastErrorShown){
        lastErrorShown = S.lastError;
        // Brand-aware message — backend already says "not a DJI Osmo camera"
        // or "not a GoPro camera". Keep it short and clear.
        toast(S.lastError);
      } else if (!S.lastError){
        lastErrorShown = '';
      }
    } else {
      console.debug("Poll skipped: HTTP "+res.status);
    }
  }catch(e){
    // Silently handle ESP32 radio contention or reboots during BLE scan
    console.debug("Poll skipped: ESP32 radio busy scanning.");
  }
  setTimeout(poll, 1500);
}
setTimeout(poll, 100);
</script>
</body>
</html>)rawliteral";

#endif // WEB_ASSETS_H
