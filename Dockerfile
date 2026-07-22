FROM walkero/docker4amigavbcc:1.16-m68k-amd64

USER root

ADD https://aminet.net/dev/c/vbcc_target_m68k-kick13.lha \
    /tmp/vbcc_target_m68k-kick13.lha

RUN mkdir -p /tmp/kick13 \
    && cd /tmp/kick13 \
    && lha x /tmp/vbcc_target_m68k-kick13.lha \
    && cp -R vbcc_target_m68k-kick13/targets/. /opt/vbcc/targets/ \
    && rm -rf /tmp/kick13 /tmp/vbcc_target_m68k-kick13.lha

WORKDIR /work