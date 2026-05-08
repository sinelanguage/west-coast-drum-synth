#!/usr/bin/env python3
"""Generate WestCoastEditor.uidesc — vector-based UI inspired by the
Blast Beats hardware drum machine.

Design goals:
- 100% vector primitives (no bitmap dependencies). Sliders are CSlider with
  draw-back/draw-value rectangles only; LEDs and decorative shapes are CView
  rectangles; arrows/glyphs are Unicode characters in CTextLabel.
- Hardware-panel aesthetic: matte black panel with white silk-screen labels,
  red accent strip per voice, faders with light-grey caps in dark tracks.
- Logical scaling: positions stored as integer pixels at 1.0× zoom; the
  controller's stepped-zoom wrapper handles host scaling.

Layout (1100 x 720):
  TOP I/O STRIP   — y=  0..56  (decorative triangles + I/O labels + logo)
  GLOBAL STRIP    — y= 56..136 (master / body-filter / morph / preset)
  WAVE GLYPH ROW  — y=136..168 (8 wave shapes 1..8 + 5 pattern indicators)
  LANE GRID       — y=168..604 (4 rows × 2 cols, 8 lanes)
  TRANSPORT/SEQ   — y=604..720 (kit/patt/step/function/run/tempo/rec + 8 voice trigger pads)
"""

import xml.etree.ElementTree as ET

# ---------------------------------------------------------------------------
# Top-level dimensions
# ---------------------------------------------------------------------------
EDITOR_W = 1180
EDITOR_H = 800

# ---------------------------------------------------------------------------
# Color palette (Blast-Beats inspired matte black panel)
# ---------------------------------------------------------------------------
COLORS = [
    ("Backdrop",        "#05060aff"),  # outer near-black bezel
    ("PanelBlack",      "#0c0e12ff"),  # main panel face
    ("PanelDeep",       "#0f1217ff"),  # slight raised module
    ("PanelLight",      "#13161bff"),  # inset zones (faders, slots)
    ("PanelInk",        "#0a0b0eff"),  # very dark inset
    ("InsetBlack",      "#000000ff"),  # silk-screened labels background
    ("Hairline",        "#393d44ff"),  # silver silkscreen lines
    ("HairlineSoft",    "#1c1f25ff"),
    ("FaderSlot",       "#030404ff"),  # very dark fader slot
    ("FaderTrack",      "#1a1d22ff"),  # mid-grey track behind cap
    ("FaderFill",       "#cdd1d6ff"),  # bright white-ish cap
    ("KnobBase",        "#1a1d22ff"),
    ("LedOn",           "#ff4f30ff"),  # bright orange-red LED
    ("LedOnDim",        "#7a2418ff"),
    ("LedOff",          "#1d0d0eff"),  # dark dormant LED
    ("LedRing",         "#0a0a0bff"),
    ("ButtonGrey",      "#c8ccd0ff"),  # rubber button face
    ("ButtonGreyDk",    "#a4a8acff"),  # button bottom shading
    ("ButtonGreyEdge",  "#7d818aff"),  # button frame
    ("ButtonRed",       "#d12424ff"),  # the red rec button
    ("ButtonShadow",    "#04050aff"),
    ("TextBright",      "#f3f5f8ff"),
    ("TextDim",         "#aab0b6ff"),
    ("TextSubtle",      "#7c8086ff"),
    ("AccentRed",       "#ff4040ff"),  # logo accent
    ("AccentRedDim",    "#7a1818ff"),
    ("WireGold",        "#c4a662ff"),  # accent “gold” lines
    ("StripDivider",    "#2e3239ff"),
]

# ---------------------------------------------------------------------------
# Lane definitions (must match parameter mapping; do not change tag math)
# ---------------------------------------------------------------------------
LANE_COUNT = 9        # parameter banks for 9 voices (param-id math)
VISIBLE_LANES = 8     # 8 voices are shown
LANE_PARAM_COUNT = 8
LANE_EXTRA_COUNT = 6
LANE_MACRO_COUNT = 4
LANE_FILTER_COUNT = 6


def core_tag(lane, offset):   return 100 + lane * LANE_PARAM_COUNT + offset
def extra_tag(lane, offset):  return 200 + lane * LANE_EXTRA_COUNT + offset
def macro_tag(lane, offset):  return 300 + lane * LANE_MACRO_COUNT + offset
def filter_tag(lane, offset): return 400 + lane * LANE_FILTER_COUNT + offset
def led_tag(lane):            return 500 + lane
def mute_tag(lane):           return 600 + lane
def oscmix_tag(lane):         return 700 + lane


LANE_NAMES = [
    ("DS1", "BD",  "C",  "kick"),
    ("DS2", "SD",  "C#", "snare"),
    ("DS3", "LT",  "D",  "low tom"),
    ("DS4", "MT",  "D#", "mid tom"),
    ("DS5", "HT",  "E",  "hi tom"),
    ("DS6", "RS",  "F",  "rim shot"),
    ("DS7", "CP",  "F#", "clap"),
    ("DS8", "HH",  "G",  "hi hat"),
    ("DS9", "CL",  "G#", "click"),
]

LANE_GRID = [
    (0, 0), (0, 1), (0, 2), (0, 3),  # left column DS1..DS4
    (1, 0), (1, 1), (1, 2), (1, 3),  # right column DS5..DS8
]

GLOBAL_TAG_NAMES = {
    0: "Master", 1: "InternalTempo", 2: "Swing",
    3: "Run", 4: "FollowHost", 5: "Preset", 6: "Randomize",
    7: "OscFilterCutoff", 8: "OscFilterResonance", 9: "OscFilterEnv",
    10: "RandomizeAmount",
}

LANE_PREFIXES = ["Kick", "Snare", "Hat", "Perc", "PercB",
                 "PercA2", "PercB2", "RimShot", "Clap"]
CORE_SUFFIXES = ["Tune", "Decay", "Fold", "Fm", "Noise",
                 "Drive", "Level", "Pan"]
EXTRA_SUFFIXES = ["PitchEnv", "PitchEnvDecay", "Transient",
                  "NoiseTone", "NoiseDecay", "Snap"]
MACRO_SUFFIXES = ["TransientDecay", "TransientMix",
                  "NoiseResonance", "NoiseEnvAmount"]
FILTER_SUFFIXES = ["OscCutoff", "OscReso", "OscFltEnv",
                   "TransCutoff", "TransReso", "TransFltEnv"]
LED_NAMES = ["KickLed", "SnareLed", "HatLed", "PercALed", "PercA2Led",
             "PercBLed", "PercB2Led", "RimShotLed", "ClapLed"]
MUTE_NAMES = ["KickMute", "SnareMute", "HatMute", "PercAMute", "PercA2Mute",
              "PercBMute", "PercB2Mute", "RimShotMute", "ClapMute"]
OSCMIX_NAMES = ["KickOscMix", "SnareOscMix", "HatOscMix",
                "PercAOscMix", "PercA2OscMix",
                "PercBOscMix", "PercB2OscMix",
                "RimShotOscMix", "ClapOscMix"]

# ---------------------------------------------------------------------------
# Per-section visible parameters (5 sections per lane)
# ---------------------------------------------------------------------------
def section_tags(lane):
    c = lambda o: core_tag(lane, o)
    e = lambda o: extra_tag(lane, o)
    m = lambda o: macro_tag(lane, o)
    f = lambda o: filter_tag(lane, o)
    return [
        # OSC + WAVESHAPE: Tune, Decay, Fold, FM, OscMix
        ([c(0), c(1), c(2), c(3), oscmix_tag(lane)],
         ["pit", "dec", "fld", "fm", "mix"],
         [False, False, False, False, False]),
        # PITCH ENV: PitchEnvAmt, PitchEnvDecay, OscCutoff, OscReso
        ([e(0), e(1), f(0), f(1)],
         ["pit", "pdc", "ocf", "ors"],
         [False, False, False, False]),
        # TRANSIENT: TransAtk, TransDecay, TransMix, TransCutoff
        ([e(2), m(0), m(1), f(3)],
         ["atk", "tdc", "tmx", "tcf"],
         [False, False, False, False]),
        # NOISE: Noise, NoiseTone, NoiseDecay, NoiseResonance
        ([c(4), e(3), e(4), m(2)],
         ["nos", "ton", "ndc", "nrs"],
         [False, True, False, False]),
        # OUTPUT: Drive, Level, Pan
        ([c(5), c(6), c(7)],
         ["drv", "lvl", "pan"],
         [False, False, True]),
    ]

def hidden_tags(lane):
    return [
        extra_tag(lane, 5),    # Snap
        macro_tag(lane, 3),    # NoiseEnvAmount
        filter_tag(lane, 2),   # OscFltEnv
        filter_tag(lane, 4),   # TransReso
        filter_tag(lane, 5),   # TransFltEnv
        mute_tag(lane),        # Mute
    ]

# ---------------------------------------------------------------------------
# XML helpers
# ---------------------------------------------------------------------------
def ind(level):
    return "  " * level


def cview_open(x, y, w, h, bg, transparent=False):
    return (
        f'<view background-color="{bg}" '
        f'background-color-draw-style="filled and stroked" '
        f'class="CViewContainer" mouse-enabled="true" opacity="1" '
        f'origin="{x}, {y}" size="{w}, {h}" transparent="{str(transparent).lower()}"/>'
    ).replace('/>', '>')


def cview_close():
    return '</view>'


def cview_open_transparent(x, y, w, h):
    return (
        f'<view class="CViewContainer" mouse-enabled="true" opacity="1" '
        f'origin="{x}, {y}" size="{w}, {h}" transparent="true">'
    )


def rect(x, y, w, h, color, mouse=False):
    return (
        f'<view background-color="{color}" '
        f'background-color-draw-style="filled and stroked" '
        f'class="CView" mouse-enabled="{str(mouse).lower()}" opacity="1" '
        f'origin="{x}, {y}" size="{w}, {h}" transparent="false"/>'
    )


def label(x, y, w, h, text, *, font="label_micro", color="TextDim",
          bg="PanelBlack", align="center"):
    return (
        f'<view back-color="{bg}" background-offset="0, 0" '
        f'class="CTextLabel" font="{font}" font-antialias="true" '
        f'font-color="{color}" frame-width="0" mouse-enabled="false" '
        f'opacity="1" origin="{x}, {y}" size="{w}, {h}" '
        f'style-no-frame="true" text-alignment="{align}" title="{text}" '
        f'transparent="true"/>'
    )


def slider_v(tag, x, y, w, h, *, from_center=False, mouse=True,
             back_color="FaderTrack", fill_color="FaderFill",
             wheel_inc="0.02", zoom="6", default="0.5"):
    return (
        f'<view background-offset="0, 0" class="CSlider" control-tag="{tag}" '
        f'default-value="{default}" '
        f'draw-back="true" draw-back-color="{back_color}" '
        f'draw-frame="false" draw-frame-color="{back_color}" '
        f'draw-value="true" draw-value-color="{fill_color}" '
        f'draw-value-from-center="{str(from_center).lower()}" '
        f'draw-value-inverted="false" handle-offset="0, 0" '
        f'max-value="1" min-value="0" mode="free click" '
        f'mouse-enabled="{str(mouse).lower()}" opacity="1" '
        f'orientation="vertical" origin="{x}, {y}" '
        f'reverse-orientation="false" size="{w}, {h}" '
        f'transparent="false" transparent-handle="true" '
        f'wheel-inc-value="{wheel_inc}" zoom-factor="{zoom}"/>'
    )


def slider_h(tag, x, y, w, h, *, from_center=False, mouse=True,
             back_color="FaderTrack", fill_color="FaderFill",
             wheel_inc="0.02", zoom="4", default="0.5"):
    return (
        f'<view background-offset="0, 0" class="CSlider" control-tag="{tag}" '
        f'default-value="{default}" '
        f'draw-back="true" draw-back-color="{back_color}" '
        f'draw-frame="false" draw-frame-color="{back_color}" '
        f'draw-value="true" draw-value-color="{fill_color}" '
        f'draw-value-from-center="{str(from_center).lower()}" '
        f'draw-value-inverted="false" handle-offset="0, 0" '
        f'max-value="1" min-value="0" mode="free click" '
        f'mouse-enabled="{str(mouse).lower()}" opacity="1" '
        f'orientation="horizontal" origin="{x}, {y}" '
        f'reverse-orientation="false" size="{w}, {h}" '
        f'transparent="false" transparent-handle="true" '
        f'wheel-inc-value="{wheel_inc}" zoom-factor="{zoom}"/>'
    )


def hidden_slider(tag, x, y):
    return (
        f'<view background-offset="0, 0" class="CSlider" control-tag="{tag}" '
        f'default-value="0.5" draw-back="false" draw-frame="false" '
        f'draw-value="false" handle-offset="0, 0" max-value="1" min-value="0" '
        f'mode="free click" mouse-enabled="true" opacity="0" '
        f'orientation="vertical" origin="{x}, {y}" '
        f'reverse-orientation="false" size="2, 2" transparent="true" '
        f'transparent-handle="true" wheel-inc-value="0.02" zoom-factor="1"/>'
    )


def led_value(tag, x, y, w, h):
    return (
        f'<view background-offset="0, 0" class="CSlider" control-tag="{tag}" '
        f'default-value="0" draw-back="true" draw-back-color="LedOff" '
        f'draw-frame="false" draw-frame-color="LedOff" draw-value="true" '
        f'draw-value-color="LedOn" draw-value-from-center="false" '
        f'draw-value-inverted="false" handle-offset="0, 0" max-value="1" '
        f'min-value="0" mode="free click" mouse-enabled="false" opacity="1" '
        f'orientation="horizontal" origin="{x}, {y}" '
        f'reverse-orientation="false" size="{w}, {h}" '
        f'transparent="false" transparent-handle="true" '
        f'wheel-inc-value="0" zoom-factor="2"/>'
    )


def dropdown(tag, x, y, w, h):
    return (
        f'<view back-color="KnobBase" background-offset="0, 0" '
        f'class="COptionMenu" control-tag="{tag}" default-value="0" '
        f'font="label_tiny" font-antialias="true" font-color="TextBright" '
        f'frame-color="Hairline" frame-width="1" max-value="4" '
        f'menu-check-style="true" menu-popup-style="true" min-value="0" '
        f'mouse-enabled="true" opacity="1" origin="{x}, {y}" '
        f'round-rect-radius="2" size="{w}, {h}" style-round-rect="true" '
        f'text-alignment="center" transparent="false"/>'
    )


def text_button(tag, x, y, w, h, title, *, fill="ButtonGrey",
                frame="ButtonGreyDark", text_color="InsetBlack",
                font="label_tiny", radius="3"):
    return (
        f'<view back-color="{fill}" background-offset="0, 0" '
        f'class="CTextButton" control-tag="{tag}" default-value="0" '
        f'font="{font}" font-antialias="true" font-color="{text_color}" '
        f'frame-color="{frame}" frame-width="1" max-value="1" min-value="0" '
        f'mouse-enabled="true" opacity="1" origin="{x}, {y}" '
        f'round-rect-radius="{radius}" size="{w}, {h}" '
        f'style-round-rect="true" title="{title}" transparent="false"/>'
    )


# ---------------------------------------------------------------------------
# Top I/O strip — decorative jack labels with downward arrows
# ---------------------------------------------------------------------------
TOP_STRIP_H = 64

IO_LABELS_LEFT  = ["dc", "out1", "out2", "out3", "out4"]   # arrows pointing UP (outputs)
IO_LABELS_RIGHT = ["mix", "sd sync", "midi", "usb"]        # arrows pointing DOWN (inputs)

def build_top_strip():
    lines = []
    lines.append(f'{ind(2)}{rect(0, 0, EDITOR_W, TOP_STRIP_H, "PanelBlack", mouse=False)}')
    # silver hairline along bottom edge of the strip
    lines.append(f'{ind(2)}{rect(0, TOP_STRIP_H - 1, EDITOR_W, 1, "HairlineSoft")}')

    # Logo block on the right side: "WCDS" stylized, with subtitle
    logo_x = EDITOR_W - 240
    lines.append(f'{ind(2)}{label(logo_x,  6, 230, 22, "WCDS", font="label_logo", color="TextBright", align="right", bg="PanelBlack")}')
    lines.append(f'{ind(2)}{label(logo_x, 30, 230, 14, "WEST COAST DRUMS", font="label_logo_sub", color="AccentRed", align="right", bg="PanelBlack")}')

    # Centre-left: I/O ports row (decorative). Two groups separated by a small gap.
    base_y = 6
    jack_w = 22
    jack_gap = 14
    cx = 24
    for name in IO_LABELS_LEFT:
        # outer jack ring
        lines.append(f'{ind(2)}{rect(cx, base_y, jack_w, jack_w, "PanelLight")}')
        lines.append(f'{ind(2)}{rect(cx + 2, base_y + 2, jack_w - 4, jack_w - 4, "PanelInk")}')
        lines.append(f'{ind(2)}{rect(cx + 7, base_y + 7, jack_w - 14, jack_w - 14, "Backdrop")}')
        # arrow above + label below
        lines.append(f'{ind(2)}{label(cx - 4, base_y + jack_w + 1, jack_w + 8, 10, "▲", font="label_arrow", color="TextDim", bg="PanelBlack")}')
        lines.append(f'{ind(2)}{label(cx - 4, base_y + jack_w + 11, jack_w + 8, 10, name, font="label_micro", color="TextDim", bg="PanelBlack")}')
        cx += jack_w + jack_gap

    cx += jack_gap   # gap between groups

    for name in IO_LABELS_RIGHT:
        lines.append(f'{ind(2)}{rect(cx, base_y, jack_w, jack_w, "PanelLight")}')
        lines.append(f'{ind(2)}{rect(cx + 2, base_y + 2, jack_w - 4, jack_w - 4, "PanelInk")}')
        lines.append(f'{ind(2)}{rect(cx + 7, base_y + 7, jack_w - 14, jack_w - 14, "Backdrop")}')
        lines.append(f'{ind(2)}{label(cx - 4, base_y + jack_w + 1, jack_w + 8, 10, "▼", font="label_arrow", color="TextDim", bg="PanelBlack")}')
        lines.append(f'{ind(2)}{label(cx - 4, base_y + jack_w + 11, jack_w + 8, 10, name, font="label_micro", color="TextDim", bg="PanelBlack")}')
        cx += jack_w + jack_gap

    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Global strip — master / body filter / morph / preset
# ---------------------------------------------------------------------------
GLOBAL_Y = TOP_STRIP_H
GLOBAL_H = 80

def build_global_strip():
    lines = []
    y0 = GLOBAL_Y
    lines.append(f'{ind(2)}{rect(0, y0, EDITOR_W, GLOBAL_H, "PanelDeep")}')

    # Subtle silver hairline above + below
    lines.append(f'{ind(2)}{rect(0, y0,         EDITOR_W, 1, "Hairline")}')
    lines.append(f'{ind(2)}{rect(0, y0 + GLOBAL_H - 1, EDITOR_W, 1, "Hairline")}')

    # MASTER (big horizontal slider) – tag 0
    sx = 24
    lines.append(f'{ind(2)}{label(sx, y0 + 8, 80, 10, "MASTER", font="label_section", color="TextBright", bg="PanelDeep", align="left")}')
    lines.append(f'{ind(2)}{slider_h(0, sx, y0 + 24, 200, 22)}')
    lines.append(f'{ind(2)}{label(sx, y0 + 50, 200, 10, "0011010110", font="label_micro", color="TextSubtle", bg="PanelDeep", align="left")}')

    # BODY FILTER — three thin horizontal sliders stacked
    bf_x = 260
    lines.append(f'{ind(2)}{label(bf_x, y0 + 8, 250, 10, "BODY FILTER", font="label_section", color="TextBright", bg="PanelDeep", align="left")}')
    for i, (tag, name) in enumerate([(7, "cut"), (8, "res"), (9, "env")]):
        ry = y0 + 22 + i * 14
        lines.append(f'{ind(2)}{label(bf_x, ry, 22, 10, name, font="label_micro", color="TextDim", bg="PanelDeep", align="left")}')
        lines.append(f'{ind(2)}{slider_h(tag, bf_x + 24, ry, 220, 10)}')

    # MORPH (centre-bipolar slider) – tag 10
    mx = 540
    lines.append(f'{ind(2)}{label(mx, y0 + 8, 160, 10, "MORPH", font="label_section", color="TextBright", bg="PanelDeep", align="left")}')
    lines.append(f'{ind(2)}{slider_h(10, mx, y0 + 30, 160, 14, from_center=True)}')
    lines.append(f'{ind(2)}{label(mx, y0 + 50, 160, 10, "kit / bank", font="label_micro", color="TextSubtle", bg="PanelDeep", align="left")}')

    # PRESET dropdown – tag 5
    px = 720
    pw = 184
    lines.append(f'{ind(2)}{label(px, y0 + 8, pw, 10, "PRESET", font="label_section", color="TextBright", bg="PanelDeep", align="left")}')
    lines.append(f'{ind(2)}{dropdown(5, px, y0 + 26, pw, 22)}')

    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Wave glyph + pattern row (decorative)
# ---------------------------------------------------------------------------
WAVE_Y = GLOBAL_Y + GLOBAL_H
WAVE_H = 48

def waveform_glyph(cx, cy, kind, color="TextBright"):
    """Draw an 8-wide × 12-tall waveform glyph using small rectangles.
    Each glyph is rendered as a compact set of 1-pixel-tall rectangles
    along a x-grid so it still looks crisp at any zoom level (vector).
    `cx` is the centre of the cell, `cy` is the top of the drawing area.
    Returns a list of XML strings.
    """
    parts = []
    # 32x12 logical drawing space, anchored at (cx-16, cy)
    ox = cx - 16
    oy = cy

    def px(x, y, w=1, h=1):
        parts.append(rect(ox + x, oy + y, w, h, color))

    if kind == 'sine':
        # discretized sine wave
        ys = [6, 5, 4, 3, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10,
              10, 10, 9, 8, 7, 6, 5, 4, 3, 2, 2, 3, 4, 5, 6, 6]
        for x, y in enumerate(ys):
            px(x, y)
    elif kind == 'tri':
        # triangle wave (V shape)
        for x in range(0, 16):
            px(x, 2 + (x // 2))
        for x in range(16, 32):
            px(x, 10 - ((x - 16) // 2))
    elif kind == 'saw':
        # rising saw
        for x in range(0, 16):
            px(x, 10 - (x // 2))
        for x in range(16, 32):
            px(x, 10 - ((x - 16) // 2))
    elif kind == 'sqr':
        # square wave
        for x in range(0, 16):
            px(x, 2)
        for y in range(2, 10):
            px(15, y)
            px(16, y)
        for x in range(16, 32):
            px(x, 10)
    elif kind == 'pulse':
        # narrow pulse
        for x in range(0, 24):
            px(x, 10)
        for y in range(2, 10):
            px(23, y)
            px(28, y)
        for x in range(24, 28):
            px(x, 2)
        for x in range(28, 32):
            px(x, 10)
    elif kind == 'fold':
        # folded sine
        for x, y in [(0, 8), (2, 6), (4, 4), (6, 2), (8, 4), (10, 6),
                     (12, 4), (14, 2), (16, 4), (18, 6), (20, 8), (22, 6),
                     (24, 4), (26, 6), (28, 8), (30, 10)]:
            px(x, y); px(x + 1, y)
    elif kind == 'fm':
        # complex fm-style
        ys = [6, 4, 3, 4, 6, 8, 9, 8, 6, 4, 2, 3, 5, 7, 9, 10,
              9, 7, 5, 3, 4, 6, 8, 9, 8, 6, 4, 5, 6, 7, 6, 6]
        for x, y in enumerate(ys):
            px(x, y)
    elif kind == 'noise':
        # pseudo-random noise - deterministic pattern
        ys = [3, 7, 5, 9, 2, 8, 4, 10, 3, 6, 9, 4, 7, 2, 8, 5,
              10, 3, 6, 8, 4, 9, 2, 7, 5, 10, 3, 8, 6, 4, 9, 5]
        for x, y in enumerate(ys):
            px(x, y)
    return parts


def pattern_dot_grid(ox, oy, active, cell=7, pad=2):
    """A 4x4 LED dot grid showing which steps are 'on'.
    `active` is a 16-char string of 0/1.
    """
    parts = []
    for r in range(4):
        for c in range(4):
            i = r * 4 + c
            on = i < len(active) and active[i] == '1'
            x = ox + c * (cell + pad)
            y = oy + r * (cell + pad)
            parts.append(rect(x, y, cell, cell, "LedOn" if on else "LedOff"))
    return parts


# 8 wave types matching our drum synth's 8 oscillator shapes
WAVE_KINDS = ['sine', 'tri', 'saw', 'sqr', 'pulse', 'fold', 'fm', 'noise']

# 5 pattern previews — 4×4 grids with deterministic visual pattern
PATTERN_BITMAPS = [
    "1000010000100001",    # diagonal
    "1010010110100101",    # checker
    "1111000000001111",    # bookend
    "1100110000110011",    # offset
    "1111111111111111",    # all on
]


def build_wave_row():
    lines = []
    y0 = WAVE_Y
    lines.append(f'{ind(2)}{rect(0, y0, EDITOR_W, WAVE_H, "PanelBlack")}')
    lines.append(f'{ind(2)}{rect(0, y0 + WAVE_H - 1, EDITOR_W, 1, "Hairline")}')

    # 8 hand-drawn waveform glyphs (left half of the row)
    cell = 60
    cx = 30
    for i in range(8):
        # arrow pointing down to indicate "this voice uses this shape"
        lines.append(f'{ind(2)}{label(cx - 28, y0 + 0, 56, 9, "▼", font="label_arrow", color="TextSubtle", bg="PanelBlack")}')
        for r in waveform_glyph(cx, y0 + 12, WAVE_KINDS[i]):
            lines.append(f'{ind(2)}{r}')
        lines.append(f'{ind(2)}{label(cx - 28, y0 + 26, 56, 12, str(i + 1), font="label_micro", color="TextSubtle", bg="PanelBlack")}')
        cx += cell

    # 5 pattern grids (right half) - ~36px wide each
    grid_origin_x = 580
    grid_w = 32 + 24
    for i, bits in enumerate(PATTERN_BITMAPS):
        gx = grid_origin_x + i * (grid_w + 36)
        for r in pattern_dot_grid(gx, y0 + 4, bits):
            lines.append(f'{ind(2)}{r}')
        # number label below
        lines.append(f'{ind(2)}{label(gx - 8, y0 + 38, 48, 10, str(i + 1), font="label_section", color="TextDim", bg="PanelBlack", align="left")}')

    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Lane grid
# ---------------------------------------------------------------------------
LANE_GRID_Y = WAVE_Y + WAVE_H
ROW_GAP = 4
LANE_H = 110
LANE_GRID_H = 4 * LANE_H + 3 * ROW_GAP

LANE_W   = 580
COL_GAP  = 20
COL0_X   = 0
COL1_X   = LANE_W + COL_GAP   # 600
ROW_Y    = [LANE_GRID_Y + i * (LANE_H + ROW_GAP) for i in range(4)]
SEP_Y    = [ROW_Y[i] + LANE_H + ROW_GAP // 2 for i in range(3)]

ACCENT_W = 4
SEC_Y    = 22
SEC_H    = 84

# Section widths / labels (fit within LANE_W with small gaps)
SEC_WIDTHS = [134, 110, 110, 110, 96]
SEC_NAMES  = ["osc + waveshape", "pitch env", "transient", "noise", "output"]
SEC_GAP    = 4
SEC_START_X = 8

SEC_X = []
_x = SEC_START_X
for w in SEC_WIDTHS:
    SEC_X.append(_x)
    _x += w + SEC_GAP

SLIDER_W = 14
SLIDER_H = 60
SLIDER_STEP = 18
SLIDER_TOP = 14
LABEL_TOP = SLIDER_TOP + SLIDER_H + 2  # 76
LABEL_W = 16
LABEL_H = 9
SEC_TITLE_Y = 2

def slider_positions(sec_w, n):
    span = (n - 1) * SLIDER_STEP + SLIDER_W
    margin = (sec_w - span) // 2
    sx = [margin + i * SLIDER_STEP for i in range(n)]
    return sx

LED_X = LANE_W - 80
LED_Y = 8
LED_W = 60
LED_H = 5


def build_lane(lane_idx, col, row):
    lx = COL0_X if col == 0 else COL1_X
    ly = ROW_Y[row]
    ds_num, short_name, note, _ = LANE_NAMES[lane_idx]

    lines = []
    lines.append(f'{ind(2)}{cview_open(lx, ly, LANE_W, LANE_H, "PanelBlack")}')

    # Red accent strip on the very left edge
    lines.append(f'{ind(3)}{rect(0, 0, ACCENT_W, LANE_H, "AccentRed")}')

    # Lane title block — wider widths so condensed/non-condensed fonts both fit
    lines.append(f'{ind(3)}{label(10, 4, 36, 16, ds_num, font="label_title", color="TextBright", bg="PanelBlack", align="left")}')
    lines.append(f'{ind(3)}{label(48, 4, 36, 16, short_name, font="label_title", color="AccentRed", bg="PanelBlack", align="left")}')
    lines.append(f'{ind(3)}{label(86, 6, 22, 14, note, font="label_tiny", color="TextSubtle", bg="PanelBlack", align="left")}')

    # LED meter (top right of lane)
    lines.append(f'{ind(3)}{led_value(led_tag(lane_idx), LED_X, LED_Y, LED_W, LED_H)}')

    # 5 sections — add a thin silver hairline between each section
    sects = section_tags(lane_idx)
    for si, (tags, labels, centers) in enumerate(sects):
        sx = SEC_X[si]
        sw = SEC_WIDTHS[si]
        n = len(tags)
        slider_xs = slider_positions(sw, n)

        # Hairline divider just before this section (skip first)
        if si > 0:
            lines.append(f'{ind(3)}{rect(sx - 2, SEC_Y + 6, 1, SEC_H - 12, "HairlineSoft")}')

        lines.append(f'{ind(3)}{cview_open(sx, SEC_Y, sw, SEC_H, "PanelDeep")}')

        # Inset slot behind each fader to make them look like physical sliders
        for j in range(n):
            sxj = slider_xs[j]
            lines.append(f'{ind(4)}{rect(sxj - 2, SLIDER_TOP - 2, SLIDER_W + 4, SLIDER_H + 4, "FaderSlot")}')
            # tick row down the middle of the slot (silkscreen)
            mid_x = sxj + SLIDER_W // 2
            for ty in range(SLIDER_TOP + 4, SLIDER_TOP + SLIDER_H - 4, 6):
                pass  # ticks would clash with the cap; skip for now

        # Section title at top
        lines.append(f'{ind(4)}{label(2, SEC_TITLE_Y, sw - 4, 10, SEC_NAMES[si], font="label_section", color="TextBright", bg="PanelDeep", align="left")}')

        # Sliders + labels
        for j, (tag, lbl, fc) in enumerate(zip(tags, labels, centers)):
            sxj = slider_xs[j]
            lines.append(f'{ind(4)}{slider_v(tag, sxj, SLIDER_TOP, SLIDER_W, SLIDER_H, from_center=fc, back_color="FaderTrack")}')
            lines.append(f'{ind(4)}{label(sxj - 1, LABEL_TOP, LABEL_W + 2, LABEL_H, lbl, font="label_micro", color="TextDim", bg="PanelDeep", align="center")}')

        lines.append(f'{ind(3)}{cview_close()}')

    lines.append(f'{ind(2)}{cview_close()}')
    return "\n".join(lines)


def build_lane_separators():
    lines = []
    sep_w = EDITOR_W
    for sy in SEP_Y:
        lines.append(f'{ind(2)}{rect(0, sy, sep_w, 1, "HairlineSoft")}')
    # vertical separator between left/right columns
    lines.append(f'{ind(2)}{rect(LANE_W, LANE_GRID_Y, COL_GAP, LANE_GRID_H, "PanelBlack")}')
    lines.append(f'{ind(2)}{rect(LANE_W + COL_GAP // 2, LANE_GRID_Y + 4, 1, LANE_GRID_H - 8, "Hairline")}')
    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Bottom transport / sequencer strip
# ---------------------------------------------------------------------------
BOTTOM_Y = LANE_GRID_Y + LANE_GRID_H
BOTTOM_H = EDITOR_H - BOTTOM_Y    # 720 - 600 = 120


def led_dot(x, y, color="LedOff", w=10, h=5):
    return rect(x, y, w, h, color)


def rubber_button(lx, ly, w, h, color="ButtonGrey", shadow="ButtonShadow"):
    """Return XML for a rubber-button: bright top half + slightly darker bottom + thin shadow."""
    half = h // 2
    parts = []
    # body
    parts.append(rect(lx, ly, w, h, color))
    # subtle bottom shading
    parts.append(rect(lx, ly + half, w, h - half, "ButtonGreyDk"))
    # bottom shadow line under the pad
    parts.append(rect(lx, ly + h, w, 2, shadow))
    # top hairline highlight (1px) for a printed-rubber feel
    parts.append(rect(lx + 1, ly + 1, w - 2, 1, "FaderFill"))
    return "\n".join(f'{ind(2)}{p}' for p in parts)


def led_dot_round(lx, ly, w=12, h=6, color="LedOn"):
    """A small LED indicator with subtle bezel."""
    parts = []
    parts.append(rect(lx - 1, ly - 1, w + 2, h + 2, "PanelInk"))
    parts.append(rect(lx, ly, w, h, color))
    return "\n".join(f'{ind(2)}{p}' for p in parts)


def build_bottom_strip():
    lines = []
    y0 = BOTTOM_Y
    lines.append(f'{ind(2)}{rect(0, y0, EDITOR_W, BOTTOM_H, "PanelBlack")}')
    lines.append(f'{ind(2)}{rect(0, y0, EDITOR_W, 1, "Hairline")}')

    # ---- Left: transport block (3 cols × 2 rows) ----
    btn_w = 46
    btn_h = 30
    gap   = 8

    sx = 18
    # group label "song" on top, with bracket
    bracket_w = 3 * btn_w + 2 * gap
    lines.append(f'{ind(2)}{label(sx, y0 + 6, bracket_w, 11, "SONG", font="label_section", color="TextDim", bg="PanelBlack")}')
    lines.append(f'{ind(2)}{rect(sx + 8, y0 + 18, bracket_w - 16, 1, "Hairline")}')

    by_top = y0 + 24
    by_bot = y0 + 80
    led_y_top = by_top - 8
    led_y_bot = by_bot - 8

    # row 1: kit/patt, step, function
    top_names = ["kit/patt", "step", "function"]
    top_leds  = ["LedOn", "LedOn", "LedOff"]
    for i, (name, led) in enumerate(zip(top_names, top_leds)):
        bx = sx + i * (btn_w + gap)
        lines.append(led_dot_round(bx + (btn_w - 10) // 2, led_y_top, w=10, h=5, color=led))
        lines.append(rubber_button(bx, by_top, btn_w, btn_h))
        lines.append(f'{ind(2)}{label(bx, by_top + btn_h + 4, btn_w, 10, name, font="label_micro", color="TextDim", bg="PanelBlack")}')

    # row 2: run, tempo, rec(red)
    bot_names = ["run", "tempo", "rec"]
    bot_leds  = ["LedOff", "LedOff", "LedOn"]
    bot_cols  = ["ButtonGrey", "ButtonGrey", "ButtonRed"]
    for i, (name, led, col) in enumerate(zip(bot_names, bot_leds, bot_cols)):
        bx = sx + i * (btn_w + gap)
        lines.append(led_dot_round(bx + (btn_w - 10) // 2, led_y_bot, w=10, h=5, color=led))
        lines.append(rubber_button(bx, by_bot, btn_w, btn_h, color=col))
        lines.append(f'{ind(2)}{label(bx, by_bot + btn_h + 4, btn_w, 10, name, font="label_micro", color="TextDim", bg="PanelBlack")}')

    # ---- Centre: 8 voice trigger pads (top row + bottom row of step-mode pads) ----
    pad_w = 70
    pad_h = 30
    pad_gap = 6
    pad_origin = sx + 3 * (btn_w + gap) + 36
    # Group "patt/song / kit/bank" silk-screen — placed in the gap between
    # the transport block and the pads.
    silk_x = pad_origin - 70
    lines.append(f'{ind(2)}{label(silk_x, y0 + 30, 64, 10, "patt", font="label_micro", color="TextSubtle", bg="PanelBlack", align="right")}')
    lines.append(f'{ind(2)}{label(silk_x, y0 + 42, 64, 10, "song", font="label_micro", color="TextSubtle", bg="PanelBlack", align="right")}')
    lines.append(f'{ind(2)}{label(silk_x, y0 + 86, 64, 10, "kit", font="label_micro", color="TextSubtle", bg="PanelBlack", align="right")}')
    lines.append(f'{ind(2)}{label(silk_x, y0 + 98, 64, 10, "bank", font="label_micro", color="TextSubtle", bg="PanelBlack", align="right")}')

    # top row: numbered 1..8 + short name + LED + pad
    for i in range(VISIBLE_LANES):
        px = pad_origin + i * (pad_w + pad_gap)
        ds_short = LANE_NAMES[i][1].lower()
        # number label
        lines.append(f'{ind(2)}{label(px, y0 + 4, pad_w, 11, str(i + 1), font="label_section", color="TextBright", bg="PanelBlack")}')
        lines.append(f'{ind(2)}{label(px, y0 + 18, pad_w, 10, ds_short, font="label_micro", color="TextDim", bg="PanelBlack")}')
        # LED (mapped to lane led param so live triggers light up)
        lines.append(f'{ind(2)}{led_value(led_tag(i), px + (pad_w - 22) // 2, y0 + 32, 22, 6)}')
        # pad
        lines.append(rubber_button(px, by_top, pad_w, pad_h))

    # bottom row: 8 step-mode pads with sub-labels
    bottom_labels = ["save", "length", "clear", "wiggle",
                     "swing", "mute", "solo", "prob"]
    for i, name in enumerate(bottom_labels):
        px = pad_origin + i * (pad_w + pad_gap)
        led = "LedOn" if i in (0, 3) else "LedOff"
        lines.append(led_dot_round(px + (pad_w - 10) // 2, led_y_bot, w=10, h=5, color=led))
        col = "ButtonGrey" if i != 0 else "ButtonGrey"
        lines.append(rubber_button(px, by_bot, pad_w, pad_h, color=col))
        lines.append(f'{ind(2)}{label(px, by_bot + pad_h + 4, pad_w, 10, name, font="label_micro", color="TextDim", bg="PanelBlack")}')

    # underline brackets: "edit" group (1..4) and "trick" group (5..8)
    # Position the bracket BELOW the bottom text labels so they don't overlap.
    bracket_y = by_bot + pad_h + 16   # below "save / length / ..."
    edit_x0  = pad_origin
    edit_x1  = pad_origin + 4 * (pad_w + pad_gap) - pad_gap
    trick_x0 = pad_origin + 4 * (pad_w + pad_gap)
    trick_x1 = pad_origin + 8 * (pad_w + pad_gap) - pad_gap
    for x0, x1, name in [(edit_x0, edit_x1, "edit"), (trick_x0, trick_x1, "trick")]:
        # left bracket tick
        lines.append(f'{ind(2)}{rect(x0, bracket_y, 18, 1, "Hairline")}')
        # right bracket tick
        lines.append(f'{ind(2)}{rect(x1 - 18, bracket_y, 18, 1, "Hairline")}')
        # centred name
        cx = (x0 + x1) // 2 - 24
        lines.append(f'{ind(2)}{label(cx, bracket_y - 5, 48, 10, name, font="label_section", color="TextDim", bg="PanelBlack")}')

    # ---- Right: stylized BLAST-BEATS-style logo block ----
    logo_block_w = 186
    logo_x = EDITOR_W - logo_block_w - 18
    lines.append(f'{ind(2)}{rect(logo_x, y0 + 26, logo_block_w, 78, "PanelDeep")}')
    lines.append(f'{ind(2)}{rect(logo_x, y0 + 26, logo_block_w, 1, "Hairline")}')
    lines.append(f'{ind(2)}{rect(logo_x, y0 + 103, logo_block_w, 1, "Hairline")}')
    lines.append(f'{ind(2)}{label(logo_x + 8, y0 + 28, logo_block_w - 16, 38, "WCDS", font="label_logo_big", color="TextBright", bg="PanelDeep", align="right")}')
    lines.append(f'{ind(2)}{label(logo_x + 8, y0 + 70, logo_block_w - 16, 14, "DRUM SYNTH", font="label_logo_sub", color="AccentRed", bg="PanelDeep", align="right")}')
    lines.append(f'{ind(2)}{label(logo_x + 8, y0 + 86, logo_block_w - 16, 12, "VECTOR EDITION", font="label_micro", color="TextSubtle", bg="PanelDeep", align="right")}')

    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Hidden sliders for non-visible parameters (so they remain bindable to host)
# ---------------------------------------------------------------------------
def build_hidden_sliders():
    lines = []
    hx, hy = EDITOR_W - 6, 0
    idx = 0
    for tag in [1, 2, 3, 4, 6]:
        lines.append(f'{ind(2)}{hidden_slider(tag, hx + (idx % 3), hy + (idx // 3))}')
        idx += 1
    for lane in range(LANE_COUNT):
        for tag in hidden_tags(lane):
            lines.append(f'{ind(2)}{hidden_slider(tag, hx + (idx % 3), hy + (idx // 3))}')
            idx += 1
        if lane >= VISIBLE_LANES:
            for off in range(LANE_PARAM_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(core_tag(lane, off), hx + (idx % 3), hy + (idx // 3))}')
                idx += 1
            for off in range(LANE_EXTRA_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(extra_tag(lane, off), hx + (idx % 3), hy + (idx // 3))}')
                idx += 1
            for off in range(LANE_MACRO_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(macro_tag(lane, off), hx + (idx % 3), hy + (idx // 3))}')
                idx += 1
            for off in range(LANE_FILTER_COUNT):
                lines.append(f'{ind(2)}{hidden_slider(filter_tag(lane, off), hx + (idx % 3), hy + (idx // 3))}')
                idx += 1
            lines.append(f'{ind(2)}{hidden_slider(led_tag(lane), hx + (idx % 3), hy + (idx // 3))}')
            idx += 1
            lines.append(f'{ind(2)}{hidden_slider(oscmix_tag(lane), hx + (idx % 3), hy + (idx // 3))}')
            idx += 1
    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Control tags
# ---------------------------------------------------------------------------
def build_control_tags():
    lines = [f'{ind(1)}<control-tags>']
    for tag, name in sorted(GLOBAL_TAG_NAMES.items()):
        lines.append(f'{ind(2)}<control-tag name="{name}" tag="{tag}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(CORE_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{core_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(EXTRA_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{extra_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(MACRO_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{macro_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        prefix = LANE_PREFIXES[lane]
        for off, suffix in enumerate(FILTER_SUFFIXES):
            lines.append(f'{ind(2)}<control-tag name="{prefix}{suffix}" tag="{filter_tag(lane, off)}"/>')
    for lane in range(LANE_COUNT):
        lines.append(f'{ind(2)}<control-tag name="{LED_NAMES[lane]}" tag="{led_tag(lane)}"/>')
    for lane in range(LANE_COUNT):
        lines.append(f'{ind(2)}<control-tag name="{MUTE_NAMES[lane]}" tag="{mute_tag(lane)}"/>')
    for lane in range(LANE_COUNT):
        lines.append(f'{ind(2)}<control-tag name="{OSCMIX_NAMES[lane]}" tag="{oscmix_tag(lane)}"/>')
    lines.append(f'{ind(1)}</control-tags>')
    return "\n".join(lines)

# ---------------------------------------------------------------------------
# Top-level generator
# ---------------------------------------------------------------------------
def generate():
    parts = []
    parts.append('<?xml version="1.0" encoding="UTF-8"?>')
    parts.append('<vstgui-ui-description version="1">')

    # Fonts
    parts.append(f'{ind(1)}<fonts>')
    fonts = [
        ("label_logo_big",  "SF Pro Condensed", 36, "true"),
        ("label_logo",      "SF Pro Condensed", 22, "true"),
        ("label_logo_sub",  "SF Pro Condensed", 11, "true"),
        ("label_title",     "SF Pro Condensed", 12, "true"),
        ("label_section",   "SF Pro Condensed",  9, "true"),
        ("label_tiny",      "SF Pro Condensed",  9, "false"),
        ("label_micro",     "SF Pro Condensed",  8, "false"),
        ("label_arrow",     "SF Pro Condensed", 10, "true"),
        ("label_wave",      "SF Pro Condensed", 14, "true"),
    ]
    for name, fname, sz, bold in fonts:
        parts.append(
            f'{ind(2)}<font name="{name}" font-name="{fname}" '
            f'alternative-font-names="SF Compact,Helvetica Neue Condensed,'
            f'Helvetica Neue,Nimbus Sans Narrow,Arial" '
            f'size="{sz}" bold="{bold}" italic="false"/>'
        )
    parts.append(f'{ind(1)}</fonts>')

    # Colors
    parts.append(f'{ind(1)}<colors>')
    for name, rgba in COLORS:
        parts.append(f'{ind(2)}<color name="{name}" rgba="{rgba}"/>')
    parts.append(f'{ind(1)}</colors>')

    parts.append('')
    parts.append(
        f'{ind(1)}<template background-color="Backdrop" '
        f'background-color-draw-style="filled and stroked" '
        f'class="CViewContainer" mouse-enabled="true" name="Editor" '
        f'opacity="1" origin="0, 0" size="{EDITOR_W}, {EDITOR_H}" '
        f'minSize="{EDITOR_W // 2}, {EDITOR_H // 2}" '
        f'maxSize="{EDITOR_W * 2}, {EDITOR_H * 2}" '
        f'transparent="false" wants-focus="false">'
    )

    parts.append(build_top_strip())
    parts.append(build_global_strip())
    parts.append(build_wave_row())

    for vi in range(VISIBLE_LANES):
        col, row = LANE_GRID[vi]
        parts.append(build_lane(vi, col, row))
    parts.append(build_lane_separators())

    parts.append(build_bottom_strip())
    parts.append(build_hidden_sliders())

    parts.append(f'{ind(1)}</template>')

    parts.append('')
    parts.append(build_control_tags())

    parts.append('</vstgui-ui-description>')
    return '\n'.join(parts)


if __name__ == '__main__':
    import sys
    out_path = sys.argv[1] if len(sys.argv) > 1 else 'resource/WestCoastEditor.uidesc'
    output = generate()
    with open(out_path, 'w', encoding='utf-8') as f:
        f.write(output)
        f.write('\n')
    print(f"Generated {out_path} ({len(output)} chars, "
          f"{EDITOR_W}x{EDITOR_H})")
