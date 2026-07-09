import { StyleSheet, Text, TouchableOpacity, View } from "react-native";

const COLORS = [
  "200, 100%, 50%",
  "120, 100%, 50%",
  "0, 100%, 50%",
  "300, 100%, 50%",
  "60, 100%, 50%",
];

export default function ColorPicker({ onSelect, onClose }: any) {
  return (
    <View style={styles.modalContainer}>
      <Text style={styles.title}>Pick a Color</Text>
      <View style={styles.grid}>
        {COLORS.map((c) => (
          <TouchableOpacity
            key={c}
            style={[styles.swatch, { backgroundColor: `hsl(${c})` }]}
            onPress={() => onSelect(c)}
          />
        ))}
        <TouchableOpacity
          style={[styles.swatch, { backgroundColor: "#FFFFFF" }]}
          onPress={() => onSelect("0, 0%, 100%")}
        />
      </View>
      <TouchableOpacity style={styles.closeBtn} onPress={onClose}>
        <Text style={{ color: "#fff" }}>Close</Text>
      </TouchableOpacity>
    </View>
  );
}

const styles = StyleSheet.create({
  modalContainer: {
    flex: 1,
    backgroundColor: "#111",
    justifyContent: "center",
    alignItems: "center",
  },
  title: { color: "#fff", fontSize: 20, marginBottom: 20 },
  grid: {
    flexDirection: "row",
    flexWrap: "wrap",
    justifyContent: "center",
    width: "80%",
  },
  swatch: {
    width: 60,
    height: 60,
    margin: 10,
    borderRadius: 30,
    borderWidth: 1,
    borderColor: "#fff",
  },
  closeBtn: {
    marginTop: 40,
    padding: 15,
    backgroundColor: "#333",
    borderRadius: 10,
  },
});
