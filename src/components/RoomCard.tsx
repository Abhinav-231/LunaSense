import { Ionicons } from "@expo/vector-icons";
import Slider from "@react-native-community/slider";
import { useRef, useState } from "react";
import {
  PanResponder,
  Platform,
  Pressable,
  StyleSheet,
  Switch,
  Text,
  View,
} from "react-native";
import Svg, {
  Circle,
  Defs,
  Path,
  RadialGradient,
  Stop,
} from "react-native-svg";
import * as API from "../api";

// ─── HSV → HEX helper ────────────────────────────────────
function hsvToHex(h: number, s: number, v: number): string {
  const f = (n: number) => {
    const k = (n + h / 60) % 6;
    const val = v - v * s * Math.max(0, Math.min(k, 4 - k, 1));
    return Math.round(val * 255)
      .toString(16)
      .padStart(2, "0");
  };
  return `#${f(5)}${f(3)}${f(1)}`;
}

// ─── HSV → RGB string for display ────────────────────────
function hsvToRgbStr(h: number, s: number, v: number): string {
  const f = (n: number) => {
    const k = (n + h / 60) % 6;
    return Math.round((v - v * s * Math.max(0, Math.min(k, 4 - k, 1))) * 255);
  };
  return `rgb(${f(5)},${f(3)},${f(1)})`;
}

// ─── Color Wheel Component ────────────────────────────────
const WHEEL_SIZE = 220;
const RADIUS = WHEEL_SIZE / 2;

function ColorWheel({
  hue,
  saturation,
  onColorChange,
}: {
  hue: number;
  saturation: number;
  onColorChange: (h: number, s: number) => void;
}) {
  const wheelRef = useRef<View>(null);
  const [layout, setLayout] = useState({ x: 0, y: 0 });

  // Build the rainbow wheel using SVG arc segments
  const segments = 60;
  const paths: string[] = [];
  for (let i = 0; i < segments; i++) {
    const startAngle = (i / segments) * 360;
    const endAngle = ((i + 1) / segments) * 360;
    const s1 = Math.sin((startAngle * Math.PI) / 180);
    const c1 = Math.cos((startAngle * Math.PI) / 180);
    const s2 = Math.sin((endAngle * Math.PI) / 180);
    const c2 = Math.cos((endAngle * Math.PI) / 180);
    const x1 = RADIUS + RADIUS * c1;
    const y1 = RADIUS + RADIUS * s1;
    const x2 = RADIUS + RADIUS * c2;
    const y2 = RADIUS + RADIUS * s2;
    paths.push(
      `M ${RADIUS} ${RADIUS} L ${x1} ${y1} A ${RADIUS} ${RADIUS} 0 0 1 ${x2} ${y2} Z`,
    );
  }

  // Selector dot position
  const angle = (hue * Math.PI) / 180;
  const dist = saturation * (RADIUS - 12);
  const dotX = RADIUS + dist * Math.cos(angle);
  const dotY = RADIUS + dist * Math.sin(angle);

  const panResponder = useRef(
    PanResponder.create({
      onStartShouldSetPanResponder: () => true,
      onMoveShouldSetPanResponder: () => true,
      onPanResponderGrant: (e) =>
        handleTouch(e.nativeEvent.pageX, e.nativeEvent.pageY),
      onPanResponderMove: (e) =>
        handleTouch(e.nativeEvent.pageX, e.nativeEvent.pageY),
    }),
  ).current;

  const handleTouch = (pageX: number, pageY: number) => {
    wheelRef.current?.measure((_fx, _fy, _w, _h, px, py) => {
      const cx = pageX - px - RADIUS;
      const cy = pageY - py - RADIUS;
      const dist = Math.sqrt(cx * cx + cy * cy);
      if (dist > RADIUS) return;
      let angle = (Math.atan2(cy, cx) * 180) / Math.PI;
      if (angle < 0) angle += 360;
      const sat = Math.min(dist / RADIUS, 1);
      onColorChange(angle, sat);
    });
  };

  return (
    <View
      ref={wheelRef}
      style={{ width: WHEEL_SIZE, height: WHEEL_SIZE, alignSelf: "center" }}
      {...panResponder.panHandlers}
    >
      <Svg width={WHEEL_SIZE} height={WHEEL_SIZE}>
        <Defs>
          <RadialGradient id="white" cx="50%" cy="50%" r="50%">
            <Stop offset="0%" stopColor="#ffffff" stopOpacity="1" />
            <Stop offset="100%" stopColor="#ffffff" stopOpacity="0" />
          </RadialGradient>
        </Defs>
        {/* Hue ring */}
        {paths.map((d, i) => (
          <Path
            key={i}
            d={d}
            fill={`hsl(${(i / segments) * 360}, 100%, 50%)`}
          />
        ))}
        {/* White center overlay (saturation) */}
        <Circle cx={RADIUS} cy={RADIUS} r={RADIUS} fill="url(#white)" />
        {/* Dark overlay for value */}
        <Circle cx={RADIUS} cy={RADIUS} r={RADIUS} fill="rgba(0,0,0,0.08)" />
        {/* Selector dot */}
        <Circle
          cx={dotX}
          cy={dotY}
          r={10}
          fill={hsvToRgbStr(hue, saturation, 1)}
          stroke="#fff"
          strokeWidth={3}
        />
        <Circle
          cx={dotX}
          cy={dotY}
          r={10}
          fill="none"
          stroke="rgba(0,0,0,0.3)"
          strokeWidth={1}
        />
      </Svg>
    </View>
  );
}

// ─── MAIN ROOMCARD ────────────────────────────────────────
export default function RoomCard({ name, onDelete, isOn, toggleSwitch }: any) {
  const [hue, setHue] = useState(220); // default blue-ish
  const [saturation, setSaturation] = useState(0.6);
  const [brightness, setBrightness] = useState(80);
  const [isMonitoring, setIsMonitoring] = useState(false);
  const [showWheel, setShowWheel] = useState(false);

  const currentHex = hsvToHex(hue, saturation, 1);

  const handleColorChange = async (h: number, s: number) => {
    setHue(h);
    setSaturation(s);
    const hex = hsvToHex(h, s, 1);
    if (isOn) await API.lightColor(hex);
  };

  const handleToggle = async () => {
    toggleSwitch();
    if (!isOn) {
      await API.lightOn();
      await API.lightColor(currentHex);
    } else {
      await API.lightOff();
    }
  };

  const handleBrightness = async (val: number) => {
    setBrightness(val);
    await API.lightBrightness(Math.round(val));
  };

  return (
    <View
      style={[
        styles.card,
        {
          backgroundColor: isOn ? currentHex + "22" : "#1a1a1a",
          borderLeftColor: isOn ? currentHex : "#333",
        },
      ]}
    >
      {/* Header */}
      <View style={styles.header}>
        <View style={styles.titleRow}>
          <Text style={styles.title}>{name}</Text>
          {/* Color preview dot */}
          <View style={[styles.colorDot, { backgroundColor: currentHex }]} />
          <Switch
            value={isOn}
            onValueChange={handleToggle}
            trackColor={{ true: "#4caf50", false: "#333" }}
          />
        </View>
        <Pressable
          onPress={onDelete}
          style={({ pressed }) => pressed && styles.pressed}
        >
          <Ionicons name="trash" size={20} color="#fff" />
        </Pressable>
      </View>

      {/* Anomaly Detection */}
      <View style={styles.row}>
        <Text style={styles.label}>Anomaly Detection</Text>
        <Switch
          value={isMonitoring}
          onValueChange={setIsMonitoring}
          trackColor={{ true: "#4caf50", false: "#333" }}
        />
      </View>

      {/* Brightness Slider */}
      <View style={styles.sliderContainer}>
        <View style={styles.sliderHeader}>
          <Text style={styles.label}>Brightness</Text>
          <Text style={[styles.brightnessValue, { color: currentHex }]}>
            {Math.round(brightness)}%
          </Text>
        </View>
        <Slider
          value={brightness}
          onSlidingComplete={handleBrightness}
          minimumValue={10}
          maximumValue={100}
          minimumTrackTintColor={currentHex}
          maximumTrackTintColor="#333"
          thumbTintColor={currentHex}
        />
      </View>

      {/* Color Wheel Toggle */}
      <Pressable
        style={[styles.wheelToggleBtn, { borderColor: currentHex + "66" }]}
        onPress={() => setShowWheel(!showWheel)}
      >
        <View style={[styles.colorDotLarge, { backgroundColor: currentHex }]} />
        <Text style={[styles.wheelToggleText, { color: currentHex }]}>
          {showWheel ? "CLOSE COLOR WHEEL" : "PICK COLOR"}
        </Text>
        <Ionicons
          name={showWheel ? "chevron-up" : "chevron-down"}
          size={16}
          color={currentHex}
        />
      </Pressable>

      {/* Color Wheel */}
      {showWheel && (
        <View style={styles.wheelContainer}>
          <ColorWheel
            hue={hue}
            saturation={saturation}
            onColorChange={handleColorChange}
          />
          {/* Hex display */}
          <View style={[styles.hexBadge, { borderColor: currentHex + "55" }]}>
            <View style={[styles.hexDot, { backgroundColor: currentHex }]} />
            <Text style={[styles.hexText, { color: currentHex }]}>
              {currentHex.toUpperCase()}
            </Text>
          </View>
          {/* Quick presets */}
          <View style={styles.presetRow}>
            {[
              { label: "Warm", h: 35, s: 0.8 },
              { label: "Cool", h: 210, s: 0.5 },
              { label: "Sleep", h: 270, s: 0.4 },
              { label: "Focus", h: 60, s: 0.6 },
              { label: "White", h: 0, s: 0.0 },
            ].map((p) => (
              <Pressable
                key={p.label}
                style={[
                  styles.presetBtn,
                  { borderColor: hsvToHex(p.h, p.s, 1) + "88" },
                ]}
                onPress={() => handleColorChange(p.h, p.s)}
              >
                <View
                  style={[
                    styles.presetDot,
                    { backgroundColor: hsvToHex(p.h, p.s, 1) },
                  ]}
                />
                <Text style={styles.presetLabel}>{p.label}</Text>
              </Pressable>
            ))}
          </View>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    padding: 18,
    margin: 10,
    borderRadius: 15,
    borderLeftWidth: 4,
  },
  header: {
    flexDirection: "row",
    justifyContent: "space-between",
    alignItems: "center",
  },
  titleRow: {
    flexDirection: "row",
    alignItems: "center",
    gap: 10,
    flex: 1,
  },
  title: { color: "#fff", fontSize: 17, fontWeight: "bold" },
  colorDot: { width: 12, height: 12, borderRadius: 6 },
  colorDotLarge: { width: 16, height: 16, borderRadius: 8 },
  row: {
    flexDirection: "row",
    justifyContent: "space-between",
    alignItems: "center",
    marginTop: 14,
  },
  sliderContainer: { marginTop: 14 },
  sliderHeader: { flexDirection: "row", justifyContent: "space-between" },
  brightnessValue: { fontWeight: "bold", fontSize: 13 },
  label: { color: "#aaa", fontSize: 12, marginBottom: 4 },
  wheelToggleBtn: {
    flexDirection: "row",
    alignItems: "center",
    gap: 8,
    marginTop: 14,
    padding: 10,
    borderRadius: 10,
    borderWidth: 1,
    backgroundColor: "rgba(255,255,255,0.04)",
  },
  wheelToggleText: {
    flex: 1,
    fontSize: 12,
    fontWeight: "600",
    letterSpacing: 0.8,
  },
  wheelContainer: {
    marginTop: 14,
    alignItems: "center",
    gap: 12,
  },
  hexBadge: {
    flexDirection: "row",
    alignItems: "center",
    gap: 8,
    paddingHorizontal: 14,
    paddingVertical: 7,
    borderRadius: 20,
    borderWidth: 1,
    backgroundColor: "rgba(255,255,255,0.04)",
  },
  hexDot: { width: 10, height: 10, borderRadius: 5 },
  hexText: {
    fontFamily: Platform.OS === "ios" ? "Courier New" : "monospace",
    fontSize: 13,
    fontWeight: "600",
  },
  presetRow: {
    flexDirection: "row",
    gap: 8,
    flexWrap: "wrap",
    justifyContent: "center",
  },
  presetBtn: {
    flexDirection: "row",
    alignItems: "center",
    gap: 5,
    paddingHorizontal: 10,
    paddingVertical: 6,
    borderRadius: 20,
    borderWidth: 1,
    backgroundColor: "rgba(255,255,255,0.04)",
  },
  presetDot: { width: 8, height: 8, borderRadius: 4 },
  presetLabel: { color: "#aaa", fontSize: 11 },
  pressed: { opacity: 0.7 },
});
