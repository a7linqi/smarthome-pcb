const DEFAULTS = {
  brokerUrl: "wss://broker.emqx.io:8084/mqtt",
  deviceId: "2efe4067"
};

const COMMANDS = {
  auto: { payload: "SET_MODE:ZD", code: 1, label: "切换自动模式" },
  manual: { payload: "SET_MODE:SD", code: 2, label: "切换手动模式" },
  "pump-on": { payload: "Water_pump_ON", code: 3, label: "打开水泵" },
  "pump-off": { payload: "Water_pump_OFF", code: 4, label: "关闭水泵" }
};

const ALARMS = {
  0: "环境正常",
  1: "土壤湿度过低",
  2: "环境温度过高",
  3: "DHT11传感器异常",
  4: "水泵运行超时"
};

const elements = {
  connectionDot: document.querySelector("#connection-dot"),
  connectionText: document.querySelector("#connection-text"),
  temperature: document.querySelector("#temperature"),
  humidity: document.querySelector("#humidity"),
  soil: document.querySelector("#soil"),
  soilAdc: document.querySelector("#soil-adc"),
  modeBadge: document.querySelector("#mode-badge"),
  pumpIcon: document.querySelector("#pump-icon"),
  pumpState: document.querySelector("#pump-state"),
  alarmCard: document.querySelector("#alarm-card"),
  alarmIcon: document.querySelector("#alarm-icon"),
  alarmText: document.querySelector("#alarm-text"),
  lastUpdate: document.querySelector("#last-update"),
  feedback: document.querySelector("#command-feedback"),
  temperatureLimit: document.querySelector("#temperature-limit"),
  soilLimit: document.querySelector("#soil-limit"),
  settingsButton: document.querySelector("#settings-button"),
  settingsDialog: document.querySelector("#settings-dialog"),
  settingsForm: document.querySelector("#settings-form"),
  brokerUrl: document.querySelector("#broker-url"),
  deviceId: document.querySelector("#device-id")
};

let client = null;
let topics = null;
let lastTelemetryAt = 0;

function readSettings() {
  return {
    brokerUrl: localStorage.getItem("jiaohua.brokerUrl") || DEFAULTS.brokerUrl,
    deviceId: localStorage.getItem("jiaohua.deviceId") || DEFAULTS.deviceId
  };
}

function createTopics(deviceId) {
  const base = `jiaohua/${deviceId}`;
  return { data: `${base}/data`, control: `${base}/control`, ack: `${base}/ack` };
}

function setConnection(state, text) {
  elements.connectionDot.className = `status-dot ${state}`;
  elements.connectionText.textContent = text;
}

function setFeedback(text, state = "") {
  elements.feedback.className = `feedback ${state}`;
  elements.feedback.textContent = text;
}

function connect() {
  const settings = readSettings();
  topics = createTopics(settings.deviceId);
  elements.brokerUrl.value = settings.brokerUrl;
  elements.deviceId.value = settings.deviceId;

  if (client) {
    client.end(true);
  }

  if (!window.mqtt) {
    setConnection("offline", "MQTT组件加载失败");
    setFeedback("请检查手机网络后刷新页面", "error");
    return;
  }

  setConnection("", "正在连接服务器");
  client = mqtt.connect(settings.brokerUrl, {
    clean: true,
    connectTimeout: 5000,
    reconnectPeriod: 3000,
    clientId: `jiaohua-web-${Math.random().toString(16).slice(2, 10)}`
  });

  client.on("connect", () => {
    setConnection("", "服务器已连接，等待设备");
    client.subscribe([topics.data, topics.ack], { qos: 0 }, error => {
      setFeedback(error ? "订阅设备主题失败" : "已订阅设备数据和控制应答", error ? "error" : "success");
    });
  });

  client.on("reconnect", () => setConnection("", "正在重新连接服务器"));
  client.on("offline", () => setConnection("offline", "服务器连接断开"));
  client.on("error", error => setFeedback(`MQTT错误：${error.message}`, "error"));
  client.on("message", handleMessage);
}

function handleMessage(topic, payload) {
  let message;
  try {
    message = JSON.parse(payload.toString());
  } catch {
    setFeedback("收到无法解析的设备消息", "error");
    return;
  }

  if (topic === topics.data) {
    renderTelemetry(message);
  } else if (topic === topics.ack) {
    renderAck(message);
  }
}

function renderTelemetry(data) {
  lastTelemetryAt = Date.now();
  setConnection("online", "设备在线");
  elements.temperature.textContent = data.temperature ?? "--";
  elements.humidity.textContent = data.humidity ?? "--";
  elements.soil.textContent = data.soil ?? "--";
  elements.soilAdc.textContent = `ADC ${data.soil_adc ?? "--"}`;

  const isAuto = Number(data.mode) === 0;
  elements.modeBadge.textContent = isAuto ? "自动模式" : "手动模式";
  document.querySelectorAll("[data-command='auto'], [data-command='manual']").forEach(button => {
    button.classList.toggle("active", button.dataset.command === (isAuto ? "auto" : "manual"));
  });

  const pumpOn = Number(data.pump) === 1;
  elements.pumpState.textContent = pumpOn ? "运行中" : "关闭";
  elements.pumpIcon.src = pumpOn ? "assets/Water_pump_ON.png" : "assets/Water_pump_OFF.png";

  const alarm = Number(data.alarm ?? 0);
  elements.alarmText.textContent = ALARMS[alarm] || `未知报警 ${alarm}`;
  elements.alarmCard.classList.toggle("warning", alarm !== 0);
  elements.alarmIcon.src = alarm === 0 ? "assets/baojing_OFF.png" : "assets/baojing_ON.png";
  elements.lastUpdate.textContent = `更新于 ${new Date().toLocaleTimeString("zh-CN", { hour12: false })}`;
}

function renderAck(ack) {
  const command = Number(ack.command);
  const success = Number(ack.status) === 0;
  const label = Object.values(COMMANDS).find(item => item.code === command)?.label
    || (command === 5 ? "设置温度上限" : command === 6 ? "设置土壤湿度下限" : `命令 ${command}`);

  setFeedback(success ? `${label}成功，实际值 ${ack.value}` : `${label}被STM32拒绝`, success ? "success" : "error");
}

function publish(command, value) {
  if (!client?.connected) {
    setFeedback("服务器尚未连接，命令未发送", "error");
    return;
  }

  let spec = COMMANDS[command];
  if (command === "temp") {
    spec = { payload: `SET_T_H:${value}`, label: "设置温度上限" };
  } else if (command === "soil") {
    spec = { payload: `SET_H_L:${value}`, label: "设置土壤湿度下限" };
  }

  client.publish(topics.control, spec.payload, { qos: 0, retain: false }, error => {
    setFeedback(error ? `${spec.label}发送失败` : `${spec.label}已发送，等待STM32确认`, error ? "error" : "");
  });
}

document.querySelectorAll("[data-command]").forEach(button => {
  button.addEventListener("click", () => {
    const command = button.dataset.command;
    if (command === "temp") {
      const value = Number(elements.temperatureLimit.value);
      if (value < 10 || value > 60) return setFeedback("温度上限范围为10～60℃", "error");
      publish(command, value);
    } else if (command === "soil") {
      const value = Number(elements.soilLimit.value);
      if (value < 5 || value > 95) return setFeedback("土壤湿度下限范围为5～95%", "error");
      publish(command, value);
    } else {
      publish(command);
    }
  });
});

elements.settingsButton.addEventListener("click", () => elements.settingsDialog.showModal());
elements.settingsForm.addEventListener("submit", event => {
  event.preventDefault();
  localStorage.setItem("jiaohua.brokerUrl", elements.brokerUrl.value.trim());
  localStorage.setItem("jiaohua.deviceId", elements.deviceId.value.trim());
  elements.settingsDialog.close();
  setFeedback("连接设置已保存", "success");
  connect();
});

setInterval(() => {
  if (lastTelemetryAt && Date.now() - lastTelemetryAt > 7000) {
    setConnection("offline", "设备离线");
  }
}, 1000);

if ((location.protocol === "http:" || location.protocol === "https:") &&
    location.hostname !== "appassets.androidplatform.net" &&
    "serviceWorker" in navigator) {
  window.addEventListener("load", () => navigator.serviceWorker.register("sw.js"));
}

connect();
