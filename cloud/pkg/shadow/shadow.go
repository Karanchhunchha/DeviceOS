package shadow

import (
	"encoding/json"
	"reflect"
)

// CalculateDelta compares 'desired' state parameters against 'reported' state parameters.
// If a parameter value differs or is missing in 'reported', it is added to the delta map.
// Supports recursive comparison of nested JSON object trees.
func CalculateDelta(desired, reported map[string]interface{}) map[string]interface{} {
	delta := make(map[string]interface{})

	for k, desiredVal := range desired {
		reportedVal, exists := reported[k]
		if !exists {
			// Key is not reported by device; add to delta list
			delta[k] = desiredVal
			continue
		}

		// Handle nested objects recursively
		desiredMap, isDesiredMap := desiredVal.(map[string]interface{})
		reportedMap, isReportedMap := reportedVal.(map[string]interface{})

		if isDesiredMap && isReportedMap {
			nestedDelta := CalculateDelta(desiredMap, reportedMap)
			if len(nestedDelta) > 0 {
				delta[k] = nestedDelta
			}
		} else {
			// Compare leaf values using deep equal
			if !reflect.DeepEqual(desiredVal, reportedVal) {
				delta[k] = desiredVal
			}
		}
	}

	return delta
}

// Helper to deserialize JSON string into a map
func ParseStateMap(jsonStr string) (map[string]interface{}, error) {
	if jsonStr == "" {
		return make(map[string]interface{}), nil
	}
	var res map[string]interface{}
	if err := json.Unmarshal([]byte(jsonStr), &res); err != nil {
		return nil, err
	}
	return res, nil
}
