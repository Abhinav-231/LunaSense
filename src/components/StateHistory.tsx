import {
  forwardRef,
  useEffect,
  useImperativeHandle,
  useState,
} from "react";
import { StyleSheet, Text, TouchableOpacity, View } from "react-native";

export interface StateHistoryRef {
  clearLogs: () => void;
}

interface StateHistoryProps {
  currentStatus: string;
}

const StateHistory = forwardRef<StateHistoryRef, StateHistoryProps>(
  ({ currentStatus }, ref) => {
    const [history, setHistory] = useState<string[]>([]);
    const [showAll, setShowAll] = useState(false);

    useImperativeHandle(ref, () => ({
      clearLogs: () => setHistory([]),
    }));

    useEffect(() => {
      const timestamp = new Date().toLocaleTimeString([], {
        hour: "2-digit",
        minute: "2-digit",
      });
      setHistory((prev) => [`${currentStatus} at ${timestamp}`, ...prev]);
    }, [currentStatus]);

    const displayedHistory = showAll ? history : history.slice(0, 3);

    return (
      <View style={styles.container}>
        <Text style={styles.header}>System Activity Log</Text>
        {displayedHistory.map((item, index) => (
          <Text key={index} style={styles.logText}>{`> ${item}`}</Text>
        ))}
        {history.length > 3 && (
          <TouchableOpacity onPress={() => setShowAll(!showAll)}>
            <Text style={styles.buttonText}>
              {showAll ? "Show Less" : "View Full Logs"}
            </Text>
          </TouchableOpacity>
        )}
      </View>
    );
  },
);

const styles = StyleSheet.create({
  container: {
    backgroundColor: "#1E1E1E",
    padding: 15,
    borderRadius: 12,
    marginTop: 10,
    width: "100%",
  },
  header: {
    color: "#888",
    fontSize: 12,
    marginBottom: 8,
    textTransform: "uppercase",
  },
  logText: {
    color: "#00E676",
    fontSize: 14,
    fontFamily: "monospace",
    marginVertical: 2,
  },
  buttonText: {
    color: "#FFF",
    fontSize: 12,
    textDecorationLine: "underline",
    marginTop: 10,
  },
});

export default StateHistory;
