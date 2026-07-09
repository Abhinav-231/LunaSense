import GeneralTab from "@/components/GeneralTab";
import AsyncStorage from "@react-native-async-storage/async-storage";
import { useEffect, useState } from "react";
import {
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  TouchableOpacity,
  View,
} from "react-native";
import { SafeAreaView } from "react-native-safe-area-context";
import ClimateCard from "../components/ClimateCard";
import RoomCard from "../components/RoomCard";
import SavingsCard from "../components/SavingsCard";

export default function Dashboard() {
  const [activeTab, setActiveTab] = useState("LIGHTS");
  const [lightRooms, setLightRooms] = useState<any[]>([]);
  const [climateRooms, setClimateRooms] = useState<any[]>([]);
  const [newRoomName, setNewRoomName] = useState("");
  const [espIP, setEspIP] = useState("192.168.1.50");

  useEffect(() => {
    async function load() {
      const l = await AsyncStorage.getItem("lunasense_light_rooms");
      const c = await AsyncStorage.getItem("lunasense_climate_rooms");
      if (l) setLightRooms(JSON.parse(l));
      if (c) setClimateRooms(JSON.parse(c));
    }
    load();
  }, []);

  const addRoom = () => {
    if (!newRoomName.trim()) return;
    const newRoom = { id: Date.now(), name: newRoomName, isOn: false };
    if (activeTab === "LIGHTS") setLightRooms([...lightRooms, newRoom]);
    else if (activeTab === "AC") setClimateRooms([...climateRooms, newRoom]);
    setNewRoomName("");
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.scrollContent}>
        <Text style={styles.brandTitle}>LUNASENSE</Text>

        {/* Navigation Tabs */}
        <View style={styles.grid}>
          {["LIGHTS", "AC", "SAVINGS", "GENERAL"].map((tab) => (
            <TouchableOpacity
              key={tab}
              style={[styles.btn, activeTab === tab && styles.activeBtn]}
              onPress={() => setActiveTab(tab)}
            >
              <Text style={styles.btnText}>{tab}</Text>
            </TouchableOpacity>
          ))}
        </View>

        {/* LIGHTS SECTION */}
        {activeTab === "LIGHTS" && (
          <>
            <TouchableOpacity
              style={styles.masterOffBtn}
              onPress={() =>
                setLightRooms(lightRooms.map((r) => ({ ...r, isOn: false })))
              }
            >
              <Text style={styles.btnText}>TURN ALL LIGHTS OFF</Text>
            </TouchableOpacity>
            {lightRooms.map((r) => (
              <RoomCard
                key={r.id}
                {...r}
                onDelete={() =>
                  setLightRooms(lightRooms.filter((i) => i.id !== r.id))
                }
                toggleSwitch={() =>
                  setLightRooms(
                    lightRooms.map((i) =>
                      i.id === r.id ? { ...i, isOn: !i.isOn } : i,
                    ),
                  )
                }
              />
            ))}
          </>
        )}

        {/* AC SECTION */}
        {activeTab === "AC" && (
          <>
            <TouchableOpacity
              style={styles.masterOffBtn}
              onPress={() =>
                setClimateRooms(
                  climateRooms.map((r) => ({ ...r, isOn: false })),
                )
              }
            >
              <Text style={styles.btnText}>TURN ALL AC OFF</Text>
            </TouchableOpacity>
            {climateRooms.map((r) => (
              <ClimateCard
                key={r.id}
                {...r}
                onDelete={() =>
                  setClimateRooms(climateRooms.filter((i) => i.id !== r.id))
                }
                toggleSwitch={() =>
                  setClimateRooms(
                    climateRooms.map((i) =>
                      i.id === r.id ? { ...i, isOn: !i.isOn } : i,
                    ),
                  )
                }
              />
            ))}
          </>
        )}

        {/* SAVINGS SECTION (Lights + AC) */}
        {activeTab === "SAVINGS" && (
          <View>
            <Text style={styles.title}>Energy Intelligence</Text>
            {[...lightRooms, ...climateRooms].map((r) => (
              <SavingsCard
                key={r.id}
                name={r.name}
                data={[120, 80, 150, 90, 70, 110, 60]}
                savedAmount="450.00"
              />
            ))}
          </View>
        )}

        {/* GENERAL SETTINGS */}
        {activeTab === "GENERAL" && <GeneralTab />}

        {/* ADD ROOM INPUT (Only for Lights/AC) */}
        {(activeTab === "LIGHTS" || activeTab === "AC") && (
          <View style={styles.inputContainer}>
            <TextInput
              style={styles.input}
              placeholder="New Room Name"
              value={newRoomName}
              onChangeText={setNewRoomName}
            />
            <TouchableOpacity style={styles.addBtn} onPress={addRoom}>
              <Text style={styles.btnText}>+ ADD</Text>
            </TouchableOpacity>
          </View>
        )}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: "#000" },
  scrollContent: { padding: 20 },
  brandTitle: {
    color: "#fff",
    fontSize: 28,
    fontWeight: "bold",
    textAlign: "center",
    marginBottom: 20,
  },
  grid: { flexDirection: "row", flexWrap: "wrap", gap: 10, marginBottom: 20 },
  btn: {
    backgroundColor: "#1a1a1a",
    padding: 20,
    width: "47%",
    borderRadius: 10,
    alignItems: "center",
  },
  activeBtn: { backgroundColor: "#00509d" },
  inputContainer: { flexDirection: "row", gap: 10, marginTop: 20 },
  input: {
    flex: 1,
    backgroundColor: "#1a1a1a",
    color: "#fff",
    padding: 15,
    borderRadius: 10,
  },
  addBtn: { backgroundColor: "#4caf50", padding: 15, borderRadius: 10 },
  masterOffBtn: {
    backgroundColor: "#ff4444",
    padding: 15,
    borderRadius: 10,
    marginBottom: 10,
    alignItems: "center",
  },
  settingsContainer: {
    padding: 20,
    backgroundColor: "#1a1a1a",
    borderRadius: 15,
  },
  saveBtn: {
    backgroundColor: "#00509d",
    padding: 15,
    borderRadius: 10,
    marginTop: 10,
    alignItems: "center",
  },
  title: {
    color: "#fff",
    fontSize: 18,
    marginBottom: 10,
    fontWeight: "bold",
    textAlign: "center",
  },
  btnText: { color: "#fff", fontWeight: "bold" },
});
