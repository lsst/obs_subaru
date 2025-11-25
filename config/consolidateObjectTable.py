# Reconfigure any actions that have a bands parameter
for action in config.actions:
    if hasattr(action, "bands"):
        action.bands = ["g", "r", "i", "z", "y"]
