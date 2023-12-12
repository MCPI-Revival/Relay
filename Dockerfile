FROM debian:bookworm-slim as builder
RUN apt-get update && apt-get dist-upgrade && apt-get install -y gcc g++ cmake automake
ADD . .
RUN make build
FROM debian:bookworm-slim as runtime
RUN apt-get update && apt-get dist-upgrade && apt-get install -y tini
COPY --from=builder ./build/MCPIRelay/mcpi-relay /app/mcpi-relay
RUN mkdir /data
WORKDIR /data
ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["/app/mcpi-relay"]
