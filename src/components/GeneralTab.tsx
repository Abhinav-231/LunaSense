import AsyncStorage from "@react-native-async-storage/async-storage";
import { useEffect, useState } from "react";
import {
  Alert,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  TouchableOpacity,
  View,
} from "react-native";

export default function GeneralTab() {
  const [lightInactivityMins, setLightMins] = useState("15");
  const [acEmptyMins, setAcMins] = useState("10");
  const [saving, setSaving] = useState(false);

  useEffect(() => {
    async function load() {
      const light = await AsyncStorage.getItem("lunasense_light_timer");
      const ac = await AsyncStorage.getItem("lunasense_ac_timer");
      const ip = await AsyncStorage.getItem("lunasense_esp_ip");
      if (light) setLightMins(light);
      if (ac) setAcMins(ac);
    }
    load();
  }, []);

  const saveSettings = async () => {
    if (
      isNaN(Number(lightInactivityMins)) ||
      Number(lightInactivityMins) <= 0
    ) {
      Alert.alert("Error", "Enter a valid light timer (minutes > 0).");
      return;
    }
    if (isNaN(Number(acEmptyMins)) || Number(acEmptyMins) <= 0) {
      Alert.alert("Error", "Enter a valid AC timer (minutes > 0).");
      return;
    }

    setSaving(true);
    try {
      await AsyncStorage.setItem(
        "lunasense_light_timer",
        lightInactivityMins.trim(),
      );
      await AsyncStorage.setItem("lunasense_ac_timer", acEmptyMins.trim());

      // Push timers to ESP32
      const ip =
        (await AsyncStorage.getItem("lunasense_esp_ip")) || "192.168.1.50";
      const res = await fetch(`http://${ip}/settings`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          lightTimer: Number(lightInactivityMins),
          acTimer: Number(acEmptyMins),
        }),
      });
      const data = await res.json();
      Alert.alert(
        data.status === "settings_saved" ? "✅ Saved" : "⚠️ Saved locally",
        data.status === "settings_saved"
          ? "Timers sent to ESP32 successfully!"
          : "Saved on phone. Check ESP32 connection.",
      );
    } catch {
      Alert.alert(
        "⚠️ Saved locally",
        "Couldn't reach ESP32. Timers saved on phone.",
      );
    }
    setSaving(false);
  };

  return (
    <ScrollView contentContainerStyle={styles.container}>
      <Text style={styles.pageTitle}>General Settings</Text>

      {/* Light Timer */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>💡 Light Auto-Off Timer</Text>
        <View style={styles.card}>
          <Text style={styles.label}>Turn off after no activity (mins)</Text>
          <TextInput
            style={styles.input}
            value={lightInactivityMins}
            onChangeText={setLightMins}
            placeholder="e.g. 15"
            placeholderTextColor="#555"
            keyboardType="numeric"
          />
          <View style={styles.infoBox}>
            <Text style={styles.infoText}>
              Light turns off after{" "}
              <Text style={styles.highlight}>
                {lightInactivityMins || "?"} min
                {lightInactivityMins === "1" ? "" : "s"}
              </Text>{" "}
              of no movement — even if someone is still inside the room. Uses
              PIR sensor for activity detection.
            </Text>
          </View>
        </View>
      </View>

      {/* AC Timer */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>❄️ AC Auto-Off Timer</Text>
        <View style={styles.card}>
          <Text style={styles.label}>Turn off after room is empty (mins)</Text>
          <TextInput
            style={styles.input}
            value={acEmptyMins}
            onChangeText={setAcMins}
            placeholder="e.g. 10"
            placeholderTextColor="#555"
            keyboardType="numeric"
          />
          <View style={styles.infoBox}>
            <Text style={styles.infoText}>
              AC turns off{" "}
              <Text style={styles.highlight}>
                {acEmptyMins || "?"} min{acEmptyMins === "1" ? "" : "s"}
              </Text>{" "}
              after the mmWave radar confirms the room is completely empty. If
              someone returns before the timer ends, AC stays on.
            </Text>
          </View>
        </View>
      </View>

      <TouchableOpacity
        style={[styles.saveBtn, saving && { opacity: 0.6 }]}
        onPress={saveSettings}
        disabled={saving}
      >
        <Text style={styles.saveBtnText}>
          {saving ? "SAVING..." : "SAVE SETTINGS"}
        </Text>
      </TouchableOpacity>
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  container: { padding: 20, paddingBottom: 40 },
  pageTitle: {
    color: "#fff",
    fontSize: 22,
    fontWeight: "bold",
    marginBottom: 24,
    textAlign: "center",
    letterSpacing: 1,
  },
  section: { marginBottom: 20 },
  sectionTitle: {
    color: "#aaa",
    fontSize: 13,
    fontWeight: "600",
    letterSpacing: 1,
    marginBottom: 8,
    textTransform: "uppercase",
  },
  card: { backgroundColor: "#1a1a1a", borderRadius: 14, padding: 16 },
  label: { color: "#ccc", fontSize: 13, marginBottom: 8 },
  input: {
    backgroundColor: "#2a2a2a",
    color: "#fff",
    padding: 13,
    borderRadius: 10,
    fontSize: 15,
    borderWidth: 1,
    borderColor: "#333",
  },
  infoBox: {
    backgroundColor: "#0d1f3c",
    borderRadius: 10,
    padding: 12,
    marginTop: 12,
    borderLeftWidth: 3,
    borderLeftColor: "#00509d",
  },
  infoText: { color: "#7aacdc", fontSize: 12, lineHeight: 18 },
  highlight: { color: "#90cdf4", fontWeight: "bold" },
  saveBtn: {
    backgroundColor: "#00509d",
    padding: 16,
    borderRadius: 12,
    alignItems: "center",
    marginTop: 8,
  },
  saveBtnText: {
    color: "#fff",
    fontWeight: "bold",
    fontSize: 15,
    letterSpacing: 1.5,
  },
});
