#include "api/api_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"
#include <cstring>

static const char* TAG = "ApiServer";
static httpd_handle_t srv      = nullptr;
static QueueHandle_t  cmd_queue = nullptr;

// Current state — updated on every POST, read on GET /api/status.
static ApiCommand current = {};

// ---------------------------------------------------------------------------
// Embedded frontend
// ---------------------------------------------------------------------------
static const char FRONTEND[] = R"html(<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LedArt</title>
<style>
*{box-sizing:border-box}
body{font-family:sans-serif;max-width:480px;margin:0 auto;padding:20px;background:#111;color:#eee}
h2{color:#e74c3c;margin-top:0}
.card{background:#1e1e1e;border-radius:8px;padding:16px;margin:12px 0}
label{font-size:13px;color:#aaa;display:block;margin-top:12px}
select{width:100%;padding:9px;background:#2a2a2a;color:#eee;border:1px solid #444;border-radius:4px;font-size:15px}
input[type=color]{width:100%;height:48px;padding:2px;background:#2a2a2a;border:1px solid #444;border-radius:4px;cursor:pointer}
.row{display:flex;align-items:center;gap:8px;margin-top:6px}
.row input[type=range]{flex:1}
.row span{min-width:32px;text-align:right;font-size:13px;color:#aaa}
.row em{min-width:28px;font-style:normal;font-size:12px;color:#666}
button{width:100%;padding:13px;background:#e74c3c;color:#fff;border:none;border-radius:6px;font-size:16px;cursor:pointer;margin-top:16px}
button:active{background:#c0392b}
#msg{text-align:center;margin-top:10px;font-size:14px;min-height:18px}
.ok{color:#2ecc71}.err{color:#e74c3c}
</style>
</head>
<body>
<h2>LedArt</h2>
<div class="card">
  <label>Pattern</label>
  <select id="pat" onchange="show()">
    <option value="gradual_shift">Gradual Shift</option>
    <option value="solid_color">Solid Color</option>
  </select>
</div>
<div class="card" id="gs">
  <label>Speed</label>
  <div class="row"><input type="range" id="spd" min="0.1" max="5" step="0.1" value="1" oninput="d('spd-v',this.value)"><span id="spd-v">1.0</span></div>
  <label>Red</label>
  <div class="row"><em>min</em><input type="range" id="r0" min="0" max="255" value="0"   oninput="d('r0v',this.value)"><span id="r0v">0</span></div>
  <div class="row"><em>max</em><input type="range" id="r1" min="0" max="255" value="255" oninput="d('r1v',this.value)"><span id="r1v">255</span></div>
  <label>Green</label>
  <div class="row"><em>min</em><input type="range" id="g0" min="0" max="255" value="0"   oninput="d('g0v',this.value)"><span id="g0v">0</span></div>
  <div class="row"><em>max</em><input type="range" id="g1" min="0" max="255" value="255" oninput="d('g1v',this.value)"><span id="g1v">255</span></div>
  <label>Blue</label>
  <div class="row"><em>min</em><input type="range" id="b0" min="0" max="255" value="0"   oninput="d('b0v',this.value)"><span id="b0v">0</span></div>
  <div class="row"><em>max</em><input type="range" id="b1" min="0" max="255" value="255" oninput="d('b1v',this.value)"><span id="b1v">255</span></div>
</div>
<div class="card" id="sc" style="display:none">
  <label>Color</label>
  <input type="color" id="col" value="#ff0000">
</div>
<button onclick="apply()">Apply</button>
<div id="msg"></div>
<script>
function d(id,v){document.getElementById(id).textContent=v}
function g(id){return document.getElementById(id)}
function gi(id){return parseInt(g(id).value)}
function gf(id){return parseFloat(g(id).value)}
function show(){
  const p=g('pat').value;
  g('gs').style.display=p==='gradual_shift'?'':'none';
  g('sc').style.display=p==='solid_color'?'':'none';
}
function hexToRgb(h){return{r:parseInt(h.slice(1,3),16),g:parseInt(h.slice(3,5),16),b:parseInt(h.slice(5,7),16)}}
async function apply(){
  const pat=g('pat').value;
  let body={pattern:pat};
  if(pat==='gradual_shift'){
    body.gradual={speed:gf('spd'),r_min:gi('r0'),r_max:gi('r1'),g_min:gi('g0'),g_max:gi('g1'),b_min:gi('b0'),b_max:gi('b1')};
  } else {
    body.solid=hexToRgb(g('col').value);
  }
  try{
    const r=await fetch('/api/pattern',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
    const m=g('msg');
    if(r.ok){m.className='ok';m.textContent='Applied!'}else{m.className='err';m.textContent='Error '+r.status}
    setTimeout(()=>m.textContent='',2000);
  }catch(e){g('msg').className='err';g('msg').textContent='Connection error'}
}
async function load(){
  try{
    const s=await(await fetch('/api/status')).json();
    g('pat').value=s.pattern; show();
    if(s.gradual){
      const gs=s.gradual;
      g('spd').value=gs.speed;        d('spd-v',gs.speed);
      g('r0').value=gs.r_min;         d('r0v',gs.r_min);
      g('r1').value=gs.r_max;         d('r1v',gs.r_max);
      g('g0').value=gs.g_min;         d('g0v',gs.g_min);
      g('g1').value=gs.g_max;         d('g1v',gs.g_max);
      g('b0').value=gs.b_min;         d('b0v',gs.b_min);
      g('b1').value=gs.b_max;         d('b1v',gs.b_max);
    }
    if(s.solid){
      const toH=n=>n.toString(16).padStart(2,'0');
      g('col').value='#'+toH(s.solid.r)+toH(s.solid.g)+toH(s.solid.b);
    }
  }catch(e){}
}
load();
</script>
</body>
</html>)html";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void jsonResponse(httpd_req_t* req, cJSON* root) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    char* str = cJSON_PrintUnformatted(root);
    httpd_resp_sendstr(req, str);
    cJSON_free(str);
    cJSON_Delete(root);
}

// ---------------------------------------------------------------------------
// Handlers
// ---------------------------------------------------------------------------
static esp_err_t handleFrontend(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, FRONTEND, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t handlePatterns(httpd_req_t* req) {
    cJSON* arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, cJSON_CreateString("gradual_shift"));
    cJSON_AddItemToArray(arr, cJSON_CreateString("solid_color"));
    jsonResponse(req, arr);
    return ESP_OK;
}

static esp_err_t handleStatus(httpd_req_t* req) {
    cJSON* root = cJSON_CreateObject();

    const char* pat_name = (current.pattern == PatternId::GRADUAL_SHIFT) ? "gradual_shift" : "solid_color";
    cJSON_AddStringToObject(root, "pattern", pat_name);

    cJSON* gs = cJSON_AddObjectToObject(root, "gradual");
    cJSON_AddNumberToObject(gs, "speed", current.gradual.speed);
    cJSON_AddNumberToObject(gs, "r_min", current.gradual.r_min);
    cJSON_AddNumberToObject(gs, "r_max", current.gradual.r_max);
    cJSON_AddNumberToObject(gs, "g_min", current.gradual.g_min);
    cJSON_AddNumberToObject(gs, "g_max", current.gradual.g_max);
    cJSON_AddNumberToObject(gs, "b_min", current.gradual.b_min);
    cJSON_AddNumberToObject(gs, "b_max", current.gradual.b_max);

    cJSON* sc = cJSON_AddObjectToObject(root, "solid");
    cJSON_AddNumberToObject(sc, "r", current.solid.r);
    cJSON_AddNumberToObject(sc, "g", current.solid.g);
    cJSON_AddNumberToObject(sc, "b", current.solid.b);

    jsonResponse(req, root);
    return ESP_OK;
}

static esp_err_t handleSetPattern(httpd_req_t* req) {
    char body[512] = {};
    int  len       = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0) { httpd_resp_send_500(req); return ESP_FAIL; }

    cJSON* root = cJSON_Parse(body);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    ApiCommand cmd = current; // start from current state

    const char* pat = cJSON_GetStringValue(cJSON_GetObjectItem(root, "pattern"));
    if (!pat) { cJSON_Delete(root); httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing pattern"); return ESP_FAIL; }

    if (strcmp(pat, "gradual_shift") == 0) {
        cmd.pattern = PatternId::GRADUAL_SHIFT;
        cJSON* gs   = cJSON_GetObjectItem(root, "gradual");
        if (gs) {
            auto n = [&](const char* k, float def) {
                cJSON* v = cJSON_GetObjectItem(gs, k);
                return v ? (float)v->valuedouble : def;
            };
            cmd.gradual.speed = n("speed", cmd.gradual.speed);
            cmd.gradual.r_min = (uint8_t)n("r_min", cmd.gradual.r_min);
            cmd.gradual.r_max = (uint8_t)n("r_max", cmd.gradual.r_max);
            cmd.gradual.g_min = (uint8_t)n("g_min", cmd.gradual.g_min);
            cmd.gradual.g_max = (uint8_t)n("g_max", cmd.gradual.g_max);
            cmd.gradual.b_min = (uint8_t)n("b_min", cmd.gradual.b_min);
            cmd.gradual.b_max = (uint8_t)n("b_max", cmd.gradual.b_max);
        }
    } else if (strcmp(pat, "solid_color") == 0) {
        cmd.pattern  = PatternId::SOLID_COLOR;
        cJSON* sc    = cJSON_GetObjectItem(root, "solid");
        if (sc) {
            cmd.solid.r = (uint8_t)cJSON_GetObjectItem(sc, "r")->valueint;
            cmd.solid.g = (uint8_t)cJSON_GetObjectItem(sc, "g")->valueint;
            cmd.solid.b = (uint8_t)cJSON_GetObjectItem(sc, "b")->valueint;
        }
    } else {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Unknown pattern");
        return ESP_FAIL;
    }

    cJSON_Delete(root);
    current = cmd;
    xQueueOverwrite(cmd_queue, &cmd);

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, "{\"ok\":true}");
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------
void ApiServer::start(QueueHandle_t queue) {
    cmd_queue = queue;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8;

    if (httpd_start(&srv, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return;
    }

    httpd_uri_t routes[] = {
        {"/",             HTTP_GET,  handleFrontend,  nullptr},
        {"/api/patterns", HTTP_GET,  handlePatterns,  nullptr},
        {"/api/status",   HTTP_GET,  handleStatus,    nullptr},
        {"/api/pattern",  HTTP_POST, handleSetPattern, nullptr},
    };
    for (auto& r : routes) httpd_register_uri_handler(srv, &r);

    ESP_LOGI(TAG, "API server started");
}

void ApiServer::stop() {
    if (srv) { httpd_stop(srv); srv = nullptr; }
}
