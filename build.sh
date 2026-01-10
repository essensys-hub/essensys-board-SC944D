#!/bin/bash

# Definition of image name
IMAGE_NAME="sc944d-builder"

# Check if MQX_PATH is set
if [ -z "$MQX_PATH" ]; then
    echo "Error: MQX_PATH environment variable is not set."
    echo "Please set MQX_PATH to your local MQX 4.0 installation directory."
    echo "Example: export MQX_PATH=/path/to/Freescale_MQX_4_0"
    exit 1
fi

# Build the Docker image if it doesn't exist
if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo "Building Docker image..."
    docker build -t $IMAGE_NAME .
fi

# Run the container
# Mounts:
# - Current directory (project) maps to /workspace/project
# - MQX_PATH maps to /workspace/mqx (Standardizing the path inside container)
echo "Running build container..."
docker run --rm -it \
    -v "$(pwd):/workspace/project" \
    -v "$MQX_PATH:/workspace/mqx" \
    -w /workspace/project \
    $IMAGE_NAME \
    /bin/bash -c "echo 'Environment ready. You can now run make (after porting the makefile).'; /bin/bash"
