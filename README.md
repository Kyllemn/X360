# XboxCDTest

Primeiro teste de hardware do projeto de CD físico para Xbox 360 RGH/JTAG.

Objetivo:
Xbox 360 DVD drive -> ATAPI -> RAM -> xboxcd.log

O teste NÃO usa cache, NÃO usa read-ahead e NÃO integra RetroArch.
O disco de teste previsto é um CD físico real de PC Engine CD.

## Estado desta entrega

O projeto está fechado como **baseline PIO de diagnóstico**. Não há uma
rotina DMA inventada: o DMA real do XeLL-Reloaded precisa ser incorporado
quando a implementação exata estiver disponível no ambiente de build.

Isso é proposital. Um DMA incorreto pode travar o console e produzir um
resultado de teste sem valor.

## Build

É necessário ter o LibXenon/DEVKITXENON instalado.

    export DEVKITXENON=/usr/local/xenon
    export PATH="$DEVKITXENON/bin:$DEVKITXENON/usr/bin:$PATH"
    make

O alvo gera:

    build/xboxcdtest.elf

A conversão ELF -> XEX depende da ferramenta `elf2xex` disponível no seu
ambiente. Use:

    make ELF2XEX=/caminho/para/elf2xex

Saída esperada:

    build/xboxcdtest.xex

## Teste

1. Coloque o CD físico real de PC Engine CD no drive.
2. Execute o XEX pelo Aurora/XeXMenu/DashLaunch.
3. Aguarde o teste terminar.
4. Recupere `xboxcd.log`.
5. Faça upload do log aqui.

## O log registra

- inicialização;
- TEST UNIT READY;
- REQUEST SENSE;
- READ TOC;
- comandos READ CD;
- status/error do dispositivo;
- sense key / ASC / ASCQ;
- primeiros bytes do setor;
- verificação do sync/header;
- falhas sem travamento infinito.

## Importante

Esta primeira entrega é um **diagnóstico de transporte PIO**. O arquivo
`DMA_STATUS.md` explica por que o DMA não foi falsificado. Depois que o log
do baseline estiver funcionando, substituímos somente a camada de transporte
por DMA e repetimos os mesmos testes.
