import { StyleSheet, Text, View } from "react-native";

const PresenceIndicator = ({ status }) => {
  return (
    <View style={styles.card}>
      {/* The dot color changes dynamically based on the status prop */}
      <View
        style={[
          styles.dot,
          {
            backgroundColor: status === "Room Occupied" ? "#4CAF50" : "#FF5252",
          },
        ]}
      />
      <Text style={styles.text}>{status}</Text>
    </View>
  );
};

const styles = StyleSheet.create({
  card: {
    flexDirection: "row",
    alignItems: "center",
    backgroundColor: "#1E1E1E",
    padding: 15,
    borderRadius: 12,
    marginVertical: 10,
    width: "100%",
  },
  dot: {
    width: 12,
    height: 12,
    borderRadius: 6,
    marginRight: 10,
  },
  text: {
    color: "#FFF",
    fontSize: 16,
    fontWeight: "600",
  },
});

export default PresenceIndicator;
