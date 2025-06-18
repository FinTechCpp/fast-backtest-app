from datetime import datetime


nan = float("nan")


class StreamObject:
    def set_by_name(self, attr_name, values, key, type):
        try:
            if key in values:
                # Handle empty string cases for numeric types
                if values[key] == '' and type in (int, float):
                    if type == int:
                        setattr(self, attr_name, 0)  # Default empty integer to 0
                    else:
                        setattr(self, attr_name, 0.0)  # Default empty float to 0.0
                else:
                    setattr(self, attr_name, type(values[key]))
        except (TypeError, ValueError):
            # Handle both TypeError and ValueError
            # Use default values based on type
            if type == int:
                setattr(self, attr_name, 0)
            elif type == float:
                setattr(self, attr_name, nan)  # Use nan for floats
            # For other types, we just skip (as before)

    def set_timestamp_by_name(self, attr_name, values, key):
        try:
            if key in values:
                setattr(
                    self, attr_name, datetime.fromtimestamp(int(values[key]) / 1000)
                )
        except TypeError:
            # ignore, there will be plenty of dud values
            pass
