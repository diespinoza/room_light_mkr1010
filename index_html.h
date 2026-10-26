// index_html.h — the phone page, served from flash. Plain HTML/CSS/JS, no
// framework, so it stays small and works on anything on the home WiFi.
#pragma once

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Room light</title>
<style>
:root{color-scheme:dark}
body{margin:0;font-family:system-ui,sans-serif;background:#141418;color:#eee}
main{max-width:420px;margin:0 auto;padding:20px 16px 40px}
h1{font-size:20px;margin:0 0 16px;display:flex;align-items:center;justify-content:space-between}
.card{background:#1f1f26;border-radius:14px;padding:14px 16px;margin-bottom:14px}
label{display:block;font-size:13px;color:#aaa;margin-bottom:6px}
input[type=range]{width:100%;accent-color:#f5c05a}
input[type=color]{width:100%;height:48px;border:0;background:none;padding:0}
.sw{position:relative;width:56px;height:32px}
.sw input{opacity:0;width:0;height:0}
.sw span{position:absolute;inset:0;border-radius:32px;background:#444;transition:.2s}
.sw span:before{content:"";position:absolute;width:24px;height:24px;left:4px;top:4px;border-radius:50%;background:#fff;transition:.2s}
.sw input:checked+span{background:#f5c05a}
.sw input:checked+span:before{transform:translateX(24px)}
.presets{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}
.presets button{padding:12px 0;border:0;border-radius:10px;background:#2c2c36;color:#eee;font-size:14px;text-transform:capitalize}
.presets button:active{background:#f5c05a;color:#000}
#info{font-size:12px;color:#888;text-align:center}
.off main{opacity:.55}
</style></head><body><main>
<h1>Room light <label class="sw"><input type="checkbox" id="on"><span></span></label></h1>
<div class="card"><label>Brightness</label><input type="range" id="bri" min="1" max="255"></div>
<div class="card"><label>Colour</label><input type="color" id="col">
<label style="margin-top:12px">White</label><input type="range" id="w" min="0" max="255"></div>
<div class="card"><label>Presets</label><div class="presets" id="presets"></div></div>
<div id="info">connecting…</div>
</main>
<script>
const $=id=>document.getElementById(id);
let s={},timer;
const hex=(r,g,b)=>'#'+[r,g,b].map(v=>v.toString(16).padStart(2,'0')).join('');
function render(){
  $('on').checked=s.on;$('bri').value=s.bri;$('w').value=s.w;$('col').value=hex(s.r,s.g,s.b);
  document.body.classList.toggle('off',!s.on);
  $('info').textContent=`~${s.est_ma} mA${s.limited?' · limited by cap':''}`;
}
async function get(){try{s=await(await fetch('/api/state')).json();render();}catch(e){$('info').textContent='no connection';}}
function send(o){clearTimeout(timer);timer=setTimeout(async()=>{
  try{s=await(await fetch('/api/state',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(o)})).json();render();}
  catch(e){$('info').textContent='send failed';}},120);}
$('on').onchange=e=>send({on:e.target.checked});
$('bri').oninput=e=>send({bri:+e.target.value});
$('w').oninput=e=>send({w:+e.target.value});
$('col').oninput=e=>{const h=e.target.value;send({r:parseInt(h.slice(1,3),16),g:parseInt(h.slice(3,5),16),b:parseInt(h.slice(5,7),16)});};
fetch('/api/presets').then(r=>r.json()).then(list=>{
  const box=$('presets');
  for(const name of list){const b=document.createElement('button');b.textContent=name;b.onclick=()=>send({preset:name});box.appendChild(b);}
  const off=document.createElement('button');off.textContent='off';off.onclick=()=>send({on:false});box.appendChild(off);
});
get();setInterval(()=>{if(!timer)get();},5000);
</script></body></html>
)HTML";
