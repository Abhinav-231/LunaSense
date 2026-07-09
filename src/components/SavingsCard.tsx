import { Ionicons } from "@expo/vector-icons";
import { useState } from "react";
import {
  Dimensions,
  StyleSheet,
  Text,
  TouchableOpacity,
  View,
} from "react-native";
import { BarChart } from "react-native-chart-kit";

export default function SavingsCard({ name, data, savedAmount }: any) {
  const [expanded, setExpanded] = useState(false);

  return (
    <View style={styles.card}>
      <TouchableOpacity
        style={styles.header}
        onPress={() => setExpanded(!expanded)}
      >
        <View>
          <Text style={styles.title}>{name}</Text>
          <Text style={styles.savingsText}>Saved: ₹{savedAmount}</Text>
        </View>
        <Ionicons
          name={expanded ? "chevron-up" : "chevron-down"}
          size={24}
          color="#fff"
        />
      </TouchableOpacity>

      {expanded && (
        <View style={styles.chartContainer}>
          {/* @ts-ignore */}
          <BarChart
            data={{
              labels: ["M", "T", "W", "T", "F", "S", "S"],
              datasets: [{ data: data }],
            }}
            width={Dimensions.get("window").width - 80}
            height={180}
            yAxisLabel="₹"
            yAxisSuffix=""
            chartConfig={{
              backgroundColor: "#1a1a1a",
              backgroundGradientFrom: "#1a1a1a",
              backgroundGradientTo: "#1a1a1a",
              decimalPlaces: 1,
              color: (opacity = 1) => `rgba(255, 255, 255, ${opacity})`,
              labelColor: (opacity = 1) => `rgba(255, 255, 255, ${opacity})`,
            }}
            style={{ marginVertical: 8, borderRadius: 16 }}
          />
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: "#1a1a1a",
    padding: 20,
    margin: 10,
    borderRadius: 15,
  },
  header: {
    flexDirection: "row",
    justifyContent: "space-between",
    alignItems: "center",
  },
  title: { color: "#fff", fontSize: 18, fontWeight: "bold" },
  savingsText: { color: "#4caf50", fontSize: 14, marginTop: 4 },
  chartContainer: { marginTop: 20, alignItems: "center" },
});
