import { StyleSheet, Text, View } from "react-native";
import { useLuna } from "../components/LunaContext";

export default function PresenceIndicator() {
  const { presence } = useLuna();

  if (!presence.connected) {
    return (
      <View style={[styles.pill, styles.offline]}>
        <View style={[styles.dot, { backgroundColor: "#666" }]} />
        <Text style={styles.text}>ESP32 unreachable</Text>
      </View>
    );
  }

  const color = presence.roomOccupied ? "#22c55e" : "#888";
  const label = presence.roomOccupied
    ? presence.motion
      ? "Room occupied — motion"
      : "Room occupied — still"
    : "Room empty";

  return (
    <View style={styles.pill}>
      <View style={[styles.dot, { backgroundColor: color }]} />
      <Text style={styles.text}>{label}</Text>
      <Text style={styles.meta}>
        M {presence.motionEnergy} · S {presence.staticEnergy}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  pill: {
    flexDirection: "row",
    alignItems: "center",
    gap: 8,
    backgroundColor: "rgba(255,255,255,0.06)",
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 20,
    alignSelf: "flex-start",
  },
  offline: { opacity: 0.6 },
  dot: { width: 8, height: 8, borderRadius: 4 },
  text: { color: "#fff", fontSize: 12, fontWeight: "600" },
  meta: { color: "#666", fontSize: 10, marginLeft: 4 },
});
