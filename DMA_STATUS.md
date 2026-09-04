# DMA

O projeto não contém uma implementação DMA inventada.

O Free60 XeLL antigo possui uma implementação ATAPI direta que serve como
baseline PIO. O XeLL-Reloaded documenta que seu acesso ao DVD foi otimizado
com DMA.

A camada `atapi_packet()` foi isolada em `xboxcd.c` justamente para permitir
trocar o transporte por DMA sem alterar:

- TEST UNIT READY
- REQUEST SENSE
- READ TOC
- READ CD
- logging

Quando a rotina DMA real do XeLL-Reloaded/LibXenon for incorporada, a meta é:

    XboxCD command
       |
       +-- DMA transport
       |
       +-- status/sense
       |
       +-- result

Sem mudar a interface pública do backend.
