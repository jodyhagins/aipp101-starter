# =============================================================================
# Claude Agent with Full C++ Development Environment
# =============================================================================
FROM ubuntu:24.04

ENV TZ=America/New_York
ENV DEBIAN_FRONTEND=noninteractive

# -----------------------------------------------------------------------------
# Create non-root user (required for Claude YOLO mode)
# -----------------------------------------------------------------------------
ARG USERNAME=agent
ARG USER_UID=1001
ARG USER_GID=$USER_UID
RUN groupadd --gid $USER_GID $USERNAME 2>/dev/null || true \
    && useradd --uid $USER_UID --gid $USER_GID -m -s /bin/bash $USERNAME 2>/dev/null \
        || useradd -m -s /bin/bash $USERNAME

# -----------------------------------------------------------------------------
# C++ Development Tools
# -----------------------------------------------------------------------------
RUN apt-get update && apt-get install -y \
    # Compilers
    build-essential \
    gcc \
    g++ \
    clang \
    clang-format \
    clang-tidy \
    ccache \
    # Build systems
    cmake \
    ninja-build \
    make \
    autoconf \
    automake \
    libtool \
    pkg-config \
    # Debugging & profiling
    gdb \
    valgrind \
    strace \
    ltrace \
    perf-tools-unstable \
    # Static analysis
    cppcheck \
    iwyu \
    # Language server
    clangd \
    # Libraries (common)
    libboost-all-dev \
    libgtest-dev \
    libgmock-dev \
    libbenchmark-dev \
    libfmt-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    # Version control
    git \
    # Utilities
    ca-certificates \
    curl \
    gnupg \
    wget \
    jq \
    bash \
    vim \
    less \
    tree \
    htop \
    fd-find \
    uuid-runtime \
    bash-completion \
    # For Claude Code
    nodejs \
    npm \
    # Python (for scripts)
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*


# -----------------------------------------------------------------------------
# Update to the timezone (in TZ from above)
# -----------------------------------------------------------------------------
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# -----------------------------------------------------------------------------
# Install more recent CMake (if needed)
# -----------------------------------------------------------------------------
RUN pip3 install --break-system-packages cmake --upgrade || true

# -----------------------------------------------------------------------------
# Build custom wjh-format (clang-format from custom LLVM branch)
# ADD fetches commit metadata; cache invalidates automatically when SHA changes
# -----------------------------------------------------------------------------
ARG WJH_FORMAT_REF=wjh-format
ADD https://api.github.com/repos/jodyhagins/llvm-project/commits/${WJH_FORMAT_REF} /tmp/wjh-format-version.json
RUN git clone --depth 1 --branch ${WJH_FORMAT_REF} https://github.com/jodyhagins/llvm-project.git /tmp/llvm-project \
    && cmake -S /tmp/llvm-project/llvm -B /tmp/llvm-project/build \
       -DCMAKE_BUILD_TYPE=MinSizeRel \
       -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
       -G Ninja \
    && ninja -C /tmp/llvm-project/build clang-format \
    && cp /tmp/llvm-project/build/bin/clang-format /usr/local/bin/wjh-format \
    && rm -rf /tmp/llvm-project

# -----------------------------------------------------------------------------
# Install additional C++ tools via pip
# -----------------------------------------------------------------------------
RUN pip3 install --break-system-packages \
    conan \
    meson \
    gcovr \
    cpplint \
    || true

# -----------------------------------------------------------------------------
# Create non-root user (required for Claude YOLO mode)
# -----------------------------------------------------------------------------
USER $USERNAME
WORKDIR /home/$USERNAME

# -----------------------------------------------------------------------------
# Install Claude Code CLI
# -----------------------------------------------------------------------------
RUN mkdir -p /home/$USERNAME/.local/bin
RUN curl -fsSL https://claude.ai/install.sh | bash
ENV PATH="/home/$USERNAME/.local/bin:$PATH"
RUN claude --version

# -----------------------------------------------------------------------------
# Install GitHub Copilot and stuff it needs
# ----------------------------------------------------------------------------- 
# Use a local prefix for global npm installs
ENV NPM_CONFIG_PREFIX=/home/$USERNAME/.npm-global
RUN mkdir -p /home/$USERNAME/.npm-global/bin
# Add local bin to PATH
ENV PATH="/home/$USERNAME/.npm-global/bin:${PATH}"
# Now global installs go into ~/.npm-global
RUN npm install -g @github/copilot
RUN node --version && npm --version && copilot --version

# -----------------------------------------------------------------------------
# Set up workspace
# -----------------------------------------------------------------------------
WORKDIR /workspace

# Helpful aliases and functions
RUN echo 'alias ll="ls -la"' >> ~/.bashrc \
    && echo 'alias fd="fdfind"' >> ~/.bashrc \
    && echo 'source /etc/bash_completion' >> ~/.bashrc \
    && echo 'function hg { echo "${1:-WJH}_$(uuidgen | tr -d '\''-'\'')"; }' >> ~/.bashrc

CMD ["/bin/bash"]
