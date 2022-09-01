ARG VARIANT="16-buster"
FROM mcr.microsoft.com/vscode/devcontainers/dotnet:0-${VARIANT}

# xz is used by ./emsdk install 
RUN apt-get update && apt-get install xz-utils