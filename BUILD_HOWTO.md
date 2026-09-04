# Como gerar o XboxCDTest.xex real (fora deste ambiente)

Este projeto (código revisado nesta entrega) precisa do toolchain LibXenon/
DEVKITXENON, que **não pôde ser construído no ambiente sandbox usado nesta
conversa** porque o acesso de rede desse ambiente é restrito a uma lista
fixa de domínios (GitHub, PyPI, npm, crates.io, espelhos Ubuntu) e não inclui
os hosts de onde o `build-xenon-toolchain` baixa as fontes reais do GCC/
binutils/newlib (`mirror.netcologne.de`, `sourceware.org`, `zlib.net`,
`ftp.gnu.org`, `download.savannah.gnu.org`). Toda tentativa de acessar esses
hosts a partir do sandbox retornou `403 host_not_allowed`.

Em uma máquina Linux normal (sua própria, com internet livre), o caminho é:

## 1. Dependências do host (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y flex bison gcc-multilib libgmp3-dev libmpfr-dev \
    libmpc-dev texinfo git-core build-essential wget file pv
```

## 2. Baixar o LibXenon e construir o toolchain

```bash
git clone https://github.com/Free60Project/libxenon.git
cd libxenon/toolchain
export PREFIX=/usr/local/xenon
./build-xenon-toolchain toolchain   # binutils + gcc + newlib (demorado, ~1-2h)
./build-xenon-toolchain libxenon    # compila e instala a libxenon
```

Alternativa mais rápida (Docker, evita compilar o GCC do zero):

```bash
docker pull free60/libxenon:latest
docker run -it -v "$PWD":/app free60/libxenon:latest
```

## 3. Variáveis de ambiente

```bash
export DEVKITXENON=/usr/local/xenon
export PATH="$DEVKITXENON/bin:$DEVKITXENON/usr/bin:$PATH"
which xenon-gcc xenon-g++ xenon-ld xenon-objcopy
xenon-gcc --version
```

## 4. elf2xex

O `build-xenon-toolchain` **não gera `elf2xex`** (confirmado ao inspecionar o
script nesta sessão — ele só cuida de binutils/gcc/newlib/libxenon). Você
precisa obter o `elf2xex` separadamente a partir das ferramentas do
ecossistema Free60/XeXMenu/Xenon (normalmente distribuído junto com pacotes
prontos do "devkitXenon" ou com o XeXTool). Confirme com:

```bash
elf2xex --help
```

antes de assumir que a sintaxe bate com o Makefile deste projeto.

## 5. Compilar este projeto

```bash
cd XboxCDTest_project
make clean
make ELF2XEX=/caminho/para/elf2xex
file build/xboxcdtest.elf
file build/xboxcdtest.xex
```

## 6. Testar em hardware real

Sem um Xbox 360 RGH/JTAG físico, nenhuma etapa acima pode ser validada como
"funciona no console" — apenas como "compila sem erro". Depois de gravar o
XEX e rodar com um CD de PC Engine CD real no drive, recupere `xboxcd.log`
(ou `/xboxcd.log`) e envie de volta para análise dos resultados de TEST UNIT
READY / REQUEST SENSE / READ TOC / READ CD.
