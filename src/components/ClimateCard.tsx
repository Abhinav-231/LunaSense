import { Ionicons } from "@expo/vector-icons";
import { useState } from "react";
import { Pressable, StyleSheet, Text, View } from "react-native";
import * as API from "../api";

export default function ClimateCard({
  name,
  onDelete,
  isOn,
  toggleSwitch,
}: any) {
  const [temp, setTemp] = useState(22);
  const [activeMode, setActiveMode] = useState("Cool");
  const [activeSpeed, setActiveSpeed] = useState("Med");

  const handleOn = async () => {
    if (isOn) return;
    toggleSwitch();
    await API.acOn();
  };

  const handleOff = async () => {
    if (!isOn) return;
    toggleSwitch();
    await API.acOff();
  };

  const changeTemp = async (delta: number) => {
    const newTemp = Math.max(16, Math.min(32, temp + delta));
    setTemp(newTemp);
    await API.acSetTemp(newTemp);
  };

  const handleMode = async (mode: string) => {
    setActiveMode(mode);
    await API.acSetMode(mode);
  };

  const handleSpeed = async (speed: string) => {
    setActiveSpeed(speed);
    await API.acSetSpeed(speed);
  };

  return (
    <View
      style={[styles.card, { backgroundColor: isOn ? "#00509d" : "#1a1a1a" }]}
    >
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.title}>{name}</Text>
        <Pressable onPress={onDelete}>
          <Ionicons name="trash" size={20} color="#fff" />
        </Pressable>
      </View>

      {/* ON / OFF buttons */}
      <View style={styles.onOffRow}>
        <Pressable
          style={[styles.onOffBtn, isOn && styles.onBtnActive]}
          onPress={handleOn}
        >
          <Ionicons
            name="power"
            size={16}
            color={isOn ? "#fff" : "#aaa"}
            style={{ marginRight: 6 }}
          />
          <Text style={[styles.onOffText, isOn && { color: "#fff" }]}>ON</Text>
        </Pressable>

        <Pressable
          style={[styles.onOffBtn, !isOn && styles.offBtnActive]}
          onPress={handleOff}
        >
          <Ionicons
            name="power-outline"
            size={16}
            color={!isOn ? "#fff" : "#aaa"}
            style={{ marginRight: 6 }}
          />
          <Text style={[styles.onOffText, !isOn && { color: "#fff" }]}>
            OFF
          </Text>
        </Pressable>
      </View>

      {/* Temperature */}
      <View style={styles.tempContainer}>
        <Pressable style={styles.tempBtn} onPress={() => changeTemp(-1)}>
          <Text style={styles.controlText}>−</Text>
        </Pressable>
        <Text style={styles.tempValue}>{temp}°C</Text>
        <Pressable style={styles.tempBtn} onPress={() => changeTemp(1)}>
          <Text style={styles.controlText}>+</Text>
        </Pressable>
      </View>

      {/* Mode */}
      <View style={styles.row}>
        {["Cool", "Heat", "Fan"].map((mode) => (
          <Pressable
            key={mode}
            style={[styles.btn, activeMode === mode && styles.activeBtn]}
            onPress={() => handleMode(mode)}
          >
            <Text style={styles.btnText}>{mode}</Text>
          </Pressable>
        ))}
      </View>

      {/* Speed */}
      <View style={styles.row}>
        {["Low", "Med", "High"].map((speed) => (
          <Pressable
            key={speed}
            style={[styles.btn, activeSpeed === speed && styles.activeBtn]}
            onPress={() => handleSpeed(speed)}
          >
            <Text style={styles.btnText}>{speed}</Text>
          </Pressable>
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  card: { padding: 20, margin: 10, borderRadius: 15 },
  header: {
    flexDirection: "row",
    justifyContent: "space-between",
    marginBottom: 15,
  },
  title: { color: "#fff", fontSize: 18, fontWeight: "bold" },
  onOffRow: { flexDirection: "row", gap: 10, marginBottom: 18 },
  onOffBtn: {
    flex: 1,
    flexDirection: "row",
    alignItems: "center",
    justifyContent: "center",
    paddingVertical: 12,
    borderRadius: 10,
    backgroundColor: "rgba(255,255,255,0.08)",
    borderWidth: 1,
    borderColor: "rgba(255,255,255,0.12)",
  },
  onBtnActive: { backgroundColor: "#22c55e", borderColor: "#22c55e" },
  offBtnActive: { backgroundColor: "#ef4444", borderColor: "#ef4444" },
  onOffText: {
    color: "#aaa",
    fontWeight: "bold",
    fontSize: 14,
    letterSpacing: 1,
  },
  tempContainer: {
    flexDirection: "row",
    justifyContent: "center",
    alignItems: "center",
    gap: 30,
    marginBottom: 20,
  },
  tempValue: { color: "#fff", fontSize: 48, fontWeight: "bold" },
  tempBtn: {
    backgroundColor: "rgba(255,255,255,0.2)",
    padding: 15,
    borderRadius: 10,
  },
  row: { flexDirection: "row", gap: 10, marginBottom: 10 },
  btn: {
    flex: 1,
    backgroundColor: "rgba(255,255,255,0.1)",
    padding: 12,
    borderRadius: 8,
    alignItems: "center",
  },
  activeBtn: { backgroundColor: "#0077b6" },
  btnText: { color: "#fff", fontWeight: "600" },
  controlText: { color: "#fff", fontSize: 24, fontWeight: "bold" },
});
