import { StyleSheet, Text, View } from "react-native";

export default function StatsChart() {
  const data = [30, 65, 45, 90, 50];
  return (
    <View style={styles.chartContainer}>
      <Text style={styles.title}>Lighting Intensity</Text>
      <View style={styles.chart}>
        {data.map((val, i) => (
          <View key={i} style={[styles.bar, { height: val }]} />
        ))}
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  chartContainer: { backgroundColor: "#1E1E1E", padding: 20, borderRadius: 15 },
  title: { color: "#FFF", marginBottom: 20 },
  chart: {
    flexDirection: "row",
    alignItems: "flex-end",
    height: 100,
    justifyContent: "space-around",
  },
  bar: { width: 10, backgroundColor: "#4CAF50" },
});
