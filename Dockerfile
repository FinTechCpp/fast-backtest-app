FROM python:3.12-slim

# Install Miniconda
RUN apt-get update && apt-get install -y wget && \
    wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh && \
    bash miniconda.sh -b -p /opt/conda && \
    rm miniconda.sh && apt-get clean && rm -rf /var/lib/apt/lists/*

# Add conda to PATH
ENV PATH="/opt/conda/bin:$PATH"

# Install required system packages and Node.js
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    curl && \
    curl -fsSL https://deb.nodesource.com/setup_18.x | bash - && \
    apt-get install -y nodejs && \
    rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy ONLY dependency files first - critical for caching
COPY backtest_interface/requirements.txt ./requirements.txt

# Create conda environment and install dependencies
# This layer will be cached unless requirements.txt changes
RUN conda create -n backtestenv python=3.12 -y && \
    echo "source activate backtestenv" > ~/.bashrc && \
    conda install -n backtestenv -c conda-forge ta-lib -y && \
    /opt/conda/envs/backtestenv/bin/pip install --no-cache-dir -r requirements.txt && \
    conda clean -afy

COPY . .
RUN /opt/conda/envs/backtestenv/bin/pip install --no-cache-dir -e ./
# COPY lightweight-charts-python ./lightweight-charts-python
RUN /opt/conda/envs/backtestenv/bin/pip install --no-cache-dir -e ./lightweight-charts-python

# Install the npm package
RUN npm install pinets

# Now copy the rest of the application code
# This step happens AFTER dependencies are installed
# COPY backtest_interface/streamlit/ ./backtest_interface/streamlit/
# COPY backtest_interface/. .

# Expose the Streamlit port
EXPOSE 8501

# Command to run the app with the conda environment
SHELL ["/bin/bash", "-c"]
CMD source activate backtestenv && streamlit run /app/backtest_interface/streamlit/app.py --server.port=8501 --server.address=0.0.0.0