import os

# Use the environment variable to prevent hardcoding of paths
# into quantum graphs.
config.functorFile = os.path.join('$OBS_SUBARU_DIR', 'policy', 'ForcedSource.yaml')