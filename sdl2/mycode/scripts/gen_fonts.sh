#!/bin/sh
# gen_fonts.sh - 重新生成中文字体 lv_font_cjk_12 / lv_font_cjk_16
#
# 什么时候跑: UI 代码里新增了中文文字, 运行后界面出现方框(缺字)时。
# 做了什么: 扫描 src include 下所有字符串字面量, 收集全部非 ASCII 字符,
#           用 lv_font_conv 从黑体生成覆盖这些字符的 LVGL 字体 .c 文件,
#           直接输出到 src/ui/ 下, 重新编译即可。
#
# 一次性准备(只做一次):
#   mkdir -p ~/fontgen && cd ~/fontgen
#   curl -L https://npmmirror.com/mirrors/node/v20.18.0/node-v20.18.0-linux-x64.tar.xz -o node.tar.xz
#   tar -xf node.tar.xz
#   cp /mnt/c/Windows/Fonts/simhei.ttf .
#
set -e
MYCODE=$(cd "$(dirname "$0")/.." && pwd)
FONTGEN=/home/dorr/fontgen
NODE_BIN=$FONTGEN/node-v20.18.0-linux-x64/bin

# 1. 扫描 UI 相关源码, 收集全部用到的字符 -> LVGL range 串
python3 - "$MYCODE" > /tmp/font_range.txt <<'EOF'
import re, glob, sys
mycode = sys.argv[1]
paths = (glob.glob(mycode + '/src/ui/*.c')
         + glob.glob(mycode + '/include/ui/*.h')
         + glob.glob(mycode + '/src/core/*.c')
         + glob.glob(mycode + '/src/*.c'))
chars = set()
for p in paths:
    s = open(p, encoding='utf-8').read()
    for m in re.findall(r'"((?:[^"\\]|\\.)*)"', s):
        chars.update(m)
chars.update('°·')           # 温度/间隔符号
# •(U+2022) 由 montserrat 渲染, SimHei 无此字形, 排除掉
cjk = sorted(c for c in chars if ord(c) > 0x7E and ord(c) != 0x2022)
rng = ['0x20-0x7E', '0xB0', '0xB7'] + ['0x%04X' % ord(c) for c in cjk]
print(','.join(rng), end='')
EOF
echo "range entries: $(tr ',' '\n' < /tmp/font_range.txt | wc -l)"

# 2. 生成两个字号 (注意: 去掉代理环境变量, WSL 里 127.0.0.1 代理不可达)
for SIZE in 12 16; do
  PATH=$NODE_BIN:$PATH xargs -a /tmp/font_range.txt -I{} \
    env -u http_proxy -u https_proxy -u HTTP_PROXY -u HTTPS_PROXY \
    $NODE_BIN/npx -y --registry=https://registry.npmmirror.com \
    lv_font_conv@1.5.3 \
    --font $FONTGEN/simhei.ttf -r {} \
    --size $SIZE --bpp 4 --format lvgl --no-compress \
    --lv-font-name lv_font_cjk_$SIZE \
    -o $MYCODE/src/ui/lv_font_cjk_$SIZE.c
  echo "generated: src/ui/lv_font_cjk_$SIZE.c"
done

echo "完成, 重新编译工程即可 (字体文件无需手动加入构建, 会被自动收集)"
