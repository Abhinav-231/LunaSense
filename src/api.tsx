import AsyncStorage from "@react-native-async-storage/async-storage";

async function getBase(): Promise<string> {
  const ip = await AsyncStorage.getItem("lunasense_esp_ip");
  return `http://${ip || "192.168.1.9"}`;
}

async function post(path: string, params?: Record<string, string>) {
  try {
    const base = await getBase();
    let url = `${base}${path}`;
    if (params) url += `?${new URLSearchParams(params).toString()}`;
    const res = await fetch(url, { method: "POST" });
    return await res.json();
  } catch {
    console.warn("ESP32 unreachable:", path);
    return null;
  }
}

async function get(path: string) {
  try {
    const base = await getBase();
    const res = await fetch(`${base}${path}`);
    return await res.json();
  } catch {
    console.warn("ESP32 unreachable:", path);
    return null;
  }
}

// ── LIGHT ──────────────────────────────────────────────────
export const lightOn = () => post("/light/on");
export const lightOff = () => post("/light/off");
export const lightBrightness = (v: number) =>
  post("/light/brightness", { value: String(v) });
// NEW: sends the pastel hex color from RoomCard color picker
export const lightColor = (hex: string) =>
  post("/light/color", { hex: hex.replace("#", "") });

// ── AC ─────────────────────────────────────────────────────
export const acOn = () => post("/ac/on");
export const acOff = () => post("/ac/off");
export const acSetTemp = (t: number) => post("/ac/temp", { value: String(t) });
export const acSetMode = (m: string) => post("/ac/mode", { value: m });
export const acSetSpeed = (s: string) => post("/ac/speed", { value: s });

// ── SETTINGS ───────────────────────────────────────────────
export const pushSettings = async (lightTimer: number, acTimer: number) => {
  try {
    const base = await getBase();
    const res = await fetch(`${base}/settings`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ lightTimer, acTimer }),
    });
    return await res.json();
  } catch {
    return null;
  }
};

// ── STATUS ─────────────────────────────────────────────────
export const getStatus = () => get("/status");
