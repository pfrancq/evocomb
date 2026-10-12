#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
    EvoComb - Graphical interface

    evocomb_gui.py

    Graphical interface running the example programs of EvoComb
    (bin_packing_example, hierarchy_example and vlsi_example) and displaying
    their results, in the spirit of the old KDE applications (kdevhga,
    kdevvlsi, etc.).

    Copyright 2002-2026 by Pascal Francq (pascal@francq.info).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

Usage:
    python3 gui/evocomb_gui.py

The programs are searched in the build directories of the project
(cmake-build-*, build*) and in the PATH; their path can also be chosen in the
interface. The paths chosen are remembered in ~/.evocomb_gui.json.

Only the standard library is used (tkinter must be available).
"""

import colorsys
import json
import os
import queue
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

try:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
except ImportError:         # The helper functions can be used without tkinter
    tk = None


# Root of the project (the script is in <root>/gui)
ROOT = Path(__file__).resolve().parent.parent

# File where the settings are remembered
SETTINGS = Path.home() / '.evocomb_gui.json'

# Description of the applications
APPS = {
    'bin_packing': {
        'title': 'Bin packing (GGA)',
        'exe': 'bin_packing_example',
        'example': ROOT / 'examples' / 'bin_packing' / 'example.json',
        'heuristic': ('gga', 'heuristic'),
        'heuristics': ['first_fit', 'first_fit_descending'],
    },
    'hierarchy': {
        'title': 'Hiérarchie (HGA)',
        'exe': 'hierarchy_example',
        'example': ROOT / 'examples' / 'hierarchy' / 'example.json',
        'heuristic': ('hga', 'heuristic'),
        'heuristics': ['first_node'],
    },
    'vlsi': {
        'title': 'Placement VLSI (GA2D)',
        'exe': 'vlsi_example',
        'example': ROOT / 'examples' / 'vlsi' / 'example.json',
        'heuristic': ('ga2d', 'heuristic'),
        'heuristics': ['bottom_left', 'edge', 'center'],
    },
}


#------------------------------------------------------------------------------
# Helper functions (independent of tkinter)
#------------------------------------------------------------------------------

def find_executable(name):
    """Search a program in the build directories of the project and in the
    PATH. Return its path, or an empty string if it is not found."""
    names = [name + '.exe', name] if os.name == 'nt' else [name]
    candidates = []
    for pattern in ('cmake-build-*', 'build*', 'out'):
        for build in sorted(ROOT.glob(pattern)):
            if build.is_dir():
                for n in names:
                    candidates.extend(build.rglob(n))
    for path in candidates:
        if path.is_file() and os.access(path, os.X_OK):
            return str(path)
    for n in names:
        path = shutil.which(n)
        if path:
            return path
    return ''


def make_config(population, generations, threads, heuristic, key):
    """Build the configuration overriding the parameters given (the empty
    values are not set). 'key' is the pair (object, parameter) of the
    heuristic."""
    config = {}
    if population:
        config['population_size'] = int(population)
    if generations:
        config['max_generations'] = int(generations)
    if threads:
        config['ga'] = {'nb_threads': int(threads)}
    if heuristic:
        config[key[0]] = {key[1]: heuristic}
    return config


def run_program(exe, problem, config, process_holder=None):
    """Run a program on a problem with a configuration (a dictionary merged
    with the configuration of the problem). The program is executed in a
    temporary directory (where it writes its log). Return a dictionary with
    'code', 'result' (the JSON result or None), 'stderr', 'log' and
    'time'."""
    work = Path(tempfile.mkdtemp(prefix='evocomb_gui_'))
    try:
        cmd = [exe, str(Path(problem).resolve()), '-o', str(work / 'result.json')]
        if config:
            (work / 'config.json').write_text(json.dumps(config), encoding='utf-8')
            cmd += ['-c', str(work / 'config.json')]
        start = time.time()
        proc = subprocess.Popen(cmd, cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if process_holder is not None:
            process_holder.append(proc)
        out, err = proc.communicate()
        elapsed = time.time() - start
        result = None
        if proc.returncode == 0 and (work / 'result.json').exists():
            result = json.loads((work / 'result.json').read_text(encoding='utf-8'))
        log = ''
        for path in sorted(work.glob('*.log')):
            log += path.read_text(encoding='utf-8', errors='replace')
        return {'code': proc.returncode, 'result': result, 'time': elapsed,
                'stderr': (out + err).decode('utf-8', errors='replace'), 'log': log}
    finally:
        shutil.rmtree(work, ignore_errors=True)


def palette(n, saturation=0.45, value=0.95):
    """Return n distinct pastel colors."""
    colors = []
    for i in range(max(n, 1)):
        r, g, b = colorsys.hsv_to_rgb((i * 0.618033988749895) % 1.0, saturation, value)
        colors.append('#%02x%02x%02x' % (int(r * 255), int(g * 255), int(b * 255)))
    return colors


# Orientations of the 2D placement (see RPolygon::transform)
ORIENTATIONS = {
    'normal': (False, False, False),
    'mirror_x': (True, False, False),
    'mirror_y': (False, True, False),
    'mirror_xy': (True, True, False),
    'rota90': (False, False, True),
    'rota90_mirror_x': (True, False, True),
    'rota90_mirror_y': (False, True, True),
    'rota90_mirror_xy': (True, True, True),
}


def transform(pt, orientation):
    """Transform a point (a vector) according to an orientation: the mirror
    'x' negates Y, the mirror 'y' negates X, and the rotation (applied after)
    is of 90 degrees counter-clockwise."""
    mirror_x, mirror_y, rotate = ORIENTATIONS[orientation]
    x, y = pt
    if mirror_x:
        y = -y
    if mirror_y:
        x = -x
    if rotate:
        x, y = -y, x
    return x, y


def floor_center(rect):
    """Center of a rectangle [x1,y1,x2,y2], rounded down (as RRect::center)."""
    return (rect[0] + rect[2]) // 2, (rect[1] + rect[3]) // 2


def place_master(master, orientation, position):
    """Compute the placed shape of an instance of a master: its polygon and
    its pins (a list of (terminal, rect)) in absolute coordinates, as done by
    RGA2DObj and RGA2DLayout."""
    poly = master['polygon']
    # Translation so that the bounding rectangle starts at (0,0)
    mx = min(p[0] for p in poly)
    my = min(p[1] for p in poly)
    poly = [(x - mx, y - my) for x, y in poly]
    # Transformation and translation after it
    tpoly = [transform(p, orientation) for p in poly]
    ox = -min(p[0] for p in tpoly)
    oy = -min(p[1] for p in tpoly)
    px, py = position
    shape = [(x + ox + px, y + oy + py) for x, y in tpoly]
    pins = []
    for term in master.get('terminals', []):
        for pin in term.get('pins', []):
            x1, y1, x2, y2 = pin['rect']
            a = transform((x1 - mx, y1 - my), orientation)
            b = transform((x2 - mx, y2 - my), orientation)
            rect = [min(a[0], b[0]) + ox + px, min(a[1], b[1]) + oy + py,
                    max(a[0], b[0]) + ox + px, max(a[1], b[1]) + oy + py]
            pins.append((term['id'], rect))
    return shape, pins


def vlsi_scene(problem, result):
    """Build the elements to draw for a VLSI placement: the limits, the
    instances (with their polygon and pins), the terminals of the circuit and
    the connections of the nets (a minimum spanning tree between the pins,
    as computed by RGA2DLayout)."""
    masters = {m['id']: m for m in problem.get('masters', [])} if problem else {}
    board = {t['id']: [p['rect'] for p in t.get('pins', [])] for t in (problem or {}).get('terminals', [])}
    instances = []
    centers = {}
    for inst in result.get('instances', []):
        if 'position' not in inst:
            continue
        master = masters.get(inst['master'])
        if master:
            shape, pins = place_master(master, inst['orientation'], inst['position'])
        else:
            (x, y), (w, h) = inst['position'], inst['size']
            shape, pins = [(x, y), (x + w, y), (x + w, y + h), (x, y + h)], []
        instances.append({'id': inst['id'], 'master': inst['master'], 'orientation': inst['orientation'],
                          'position': inst['position'], 'size': inst['size'], 'shape': shape, 'pins': pins})
        for term, rect in pins:
            centers.setdefault((inst['id'], term), []).append(floor_center(rect))
        if not pins:
            x, y = inst['position']
            w, h = inst['size']
            centers.setdefault((inst['id'], None), []).append(((2 * x + w) // 2, (2 * y + h) // 2))
    for term, rects in board.items():
        centers[(None, term)] = [floor_center(r) for r in rects]

    # Connections of the nets
    wires = []
    for net in (problem or {}).get('nets', []):
        nodes = []
        for item in net.get('connect', []):
            key = (item.get('instance'), item['terminal'])
            pts = centers.get(key) or centers.get((item.get('instance'), None))
            if pts:
                nodes.append(pts)
        if len(nodes) < 2:
            continue

        def dist(a, b):
            best = None
            for p in nodes[a]:
                for q in nodes[b]:
                    d = abs(p[0] - q[0]) + abs(p[1] - q[1])
                    if best is None or d < best[0]:
                        best = (d, p, q)
            return best

        in_tree = {0}
        while len(in_tree) < len(nodes):
            best = None
            for a in in_tree:
                for b in range(len(nodes)):
                    if b not in in_tree:
                        d = dist(a, b)
                        if best is None or d[0] < best[0][0]:
                            best = (d, b)
            wires.append({'net': net['id'], 'weight': net.get('weight', 1), 'from': best[0][1], 'to': best[0][2]})
            in_tree.add(best[1])

    return {'size': result.get('size', [0, 0]), 'instances': instances, 'board': board,
            'wires': wires, 'box': result.get('bounding_box')}


#------------------------------------------------------------------------------
# Graphical interface
#------------------------------------------------------------------------------

if tk is not None:

    class Tooltip:
        """Bubble drawn inside a canvas near the mouse. It is drawn in the
        canvas itself (and not in a separate window), which works on all the
        platforms (the borderless windows are not always drawn on macOS)."""

        def __init__(self, canvas):
            self.canvas = canvas

        def show(self, text, x, y):
            """Show a text near a point (in canvas coordinates)."""
            c = self.canvas
            self.hide()
            label = c.create_text(x + 14, y + 12, text=text, anchor=tk.NW, justify=tk.LEFT,
                                  font=('TkDefaultFont', 10), fill='black', tags='tooltip')
            x1, y1, x2, y2 = c.bbox(label)
            # Keep the bubble inside the visible part of the canvas (above the
            # mouse if there is no room below it)
            right = c.canvasx(c.winfo_width()) - 8
            bottom = c.canvasy(c.winfo_height()) - 8
            dx = min(0, right - x2)
            dy = ((y - 12) - y2) if y2 > bottom else 0
            c.move(label, dx, dy)
            x1, y1, x2, y2 = c.bbox(label)
            back = c.create_rectangle(x1 - 4, y1 - 3, x2 + 4, y2 + 3, fill='#ffffe0', outline='#777', tags='tooltip')
            c.tag_lower(back, label)
            c.tag_raise('tooltip')

        def hide(self):
            self.canvas.delete('tooltip')


    class View(ttk.Frame):
        """Base class of the views of the results."""

        def show(self, result, problem):
            """Display a result (problem is the JSON problem, or None)."""
            raise NotImplementedError

        def summary(self, result):
            """Return the lines summarizing a result."""
            return []


    class BinPackingView(View):
        """Bins drawn as columns, with their objects stacked."""

        BIN_WIDTH = 46
        GAP = 14

        def __init__(self, master):
            super().__init__(master)
            self.canvas = tk.Canvas(self, background='white', highlightthickness=0)
            scroll = ttk.Scrollbar(self, orient=tk.HORIZONTAL, command=self.canvas.xview)
            self.canvas.configure(xscrollcommand=scroll.set)
            self.canvas.pack(fill=tk.BOTH, expand=True)
            scroll.pack(fill=tk.X)
            self.canvas.bind('<Configure>', lambda e: self.draw())
            self.canvas.bind('<Motion>', self.motion)
            self.canvas.bind('<Leave>', lambda e: self.tooltip.hide())
            self.tooltip = Tooltip(self.canvas)
            self.result = None
            self.items = {}

        def show(self, result, problem):
            self.result = result
            self.draw()

        def summary(self, r):
            lines = ['Objets : %d' % r['nb_objects'], 'Capacité : %d' % r['capacity'],
                     'Borne inférieure : %d boîtes' % r['lower_bound']]
            if 'min_bins' in r:
                lines.append('Optimum connu : %d boîtes' % r['min_bins'])
            lines += ['', 'Solution : %d boîtes' % r['nb_bins'], 'Fitness : %.4f' % r['fitness'],
                      'Générations : %d' % r['nb_generations']]
            return lines

        def draw(self):
            c = self.canvas
            c.delete('all')
            self.items.clear()
            if not self.result:
                return
            bins = self.result['bins']
            capacity = self.result['capacity']
            height = max(c.winfo_height() - 60, 100)
            top = 30
            colors = palette(self.result['nb_objects'])
            for i, b in enumerate(bins):
                x = self.GAP + i * (self.BIN_WIDTH + self.GAP)
                c.create_rectangle(x, top, x + self.BIN_WIDTH, top + height, outline='#555', width=1.5)
                y = top + height
                for obj, size in zip(b['objects'], b['sizes']):
                    h = size * height / capacity
                    item = c.create_rectangle(x + 2, y - h, x + self.BIN_WIDTH - 2, y,
                                              fill=colors[obj % len(colors)], outline='#666')
                    self.items[item] = 'Objet %d : taille %d' % (obj, size)
                    if h >= 14:
                        label = c.create_text(x + self.BIN_WIDTH / 2, y - h / 2, text=str(size), font=('TkDefaultFont', 8))
                        self.items[label] = self.items[item]
                    y -= h
                ratio = 100.0 * b['size'] / capacity
                c.create_text(x + self.BIN_WIDTH / 2, top - 10, text='%.0f %%' % ratio, font=('TkDefaultFont', 8),
                              fill='#070' if ratio >= 99.99 else '#333')
                c.create_text(x + self.BIN_WIDTH / 2, top + height + 12, text=str(i + 1), font=('TkDefaultFont', 8))
            c.configure(scrollregion=(0, 0, self.GAP + len(bins) * (self.BIN_WIDTH + self.GAP), top + height + 30))

        def motion(self, event):
            x, y = self.canvas.canvasx(event.x), self.canvas.canvasy(event.y)
            for item in reversed(self.canvas.find_overlapping(x, y, x, y)):
                if item in self.items:
                    self.tooltip.show(self.items[item], x, y)
                    return
            self.tooltip.hide()


    class HierarchyView(View):
        """Tree of the nodes (with their new attributes) and of the objects."""

        def __init__(self, master):
            super().__init__(master)
            bar = ttk.Frame(self)
            bar.pack(fill=tk.X)
            ttk.Button(bar, text='Tout déplier', command=lambda: self.expand(True)).pack(side=tk.LEFT, padx=2, pady=2)
            ttk.Button(bar, text='Tout replier', command=lambda: self.expand(False)).pack(side=tk.LEFT, padx=2, pady=2)
            panes = ttk.PanedWindow(self, orient=tk.VERTICAL)
            panes.pack(fill=tk.BOTH, expand=True)
            frame = ttk.Frame(panes)
            self.tree = ttk.Treeview(frame, columns=('objects',), selectmode='browse')
            self.tree.heading('#0', text='Nœud (nouveaux attributs) / objet')
            self.tree.heading('objects', text='Objets')
            self.tree.column('objects', width=70, anchor=tk.CENTER, stretch=False)
            self.tree.tag_configure('obj', foreground='#1a5fb4')
            scroll = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=self.tree.yview)
            self.tree.configure(yscrollcommand=scroll.set)
            self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            scroll.pack(side=tk.RIGHT, fill=tk.Y)
            panes.add(frame, weight=4)
            self.details = tk.Text(panes, height=6, wrap=tk.WORD, state=tk.DISABLED)
            panes.add(self.details, weight=1)
            self.tree.bind('<<TreeviewSelect>>', self.select)
            self.nodes = {}

        def summary(self, r):
            return ['Objets : %d' % r['nb_objects'], 'Attributs : %d' % r['nb_attributes'], '',
                    'Nœuds : %d' % r['nb_nodes'], 'Profondeur : %d' % r['depth'],
                    'Choix moyens : %.3f' % r['avg_choices'], 'Générations : %d' % r['nb_generations']]

        @staticmethod
        def count(node):
            return len(node['objects']) + sum(HierarchyView.count(c) for c in node['children'])

        def show(self, result, problem):
            self.tree.delete(*self.tree.get_children())
            self.nodes.clear()

            def add(parent, node):
                text = ' + '.join(node['new_attributes']) or '(aucun attribut)'
                item = self.tree.insert(parent, tk.END, text=text, values=(self.count(node),), open=True)
                self.nodes[item] = node
                for child in node['children']:
                    add(item, child)
                for obj in node['objects']:
                    self.tree.insert(item, tk.END, text='• ' + obj, tags=('obj',))

            for node in result['tree']:
                add('', node)
            self.set_details('')

        def expand(self, state):
            def rec(item):
                self.tree.item(item, open=state)
                for child in self.tree.get_children(item):
                    rec(child)
            for item in self.tree.get_children():
                rec(item)

        def select(self, event):
            sel = self.tree.selection()
            if not sel:
                return
            node = self.nodes.get(sel[0])
            if node is None:
                node = self.nodes.get(self.tree.parent(sel[0]))
            if node is None:
                return
            self.set_details('Attributs : ' + ', '.join(node['attributes']) + '\n' +
                             'Nouveaux : ' + ', '.join(node['new_attributes']) + '\n' +
                             'Objets : ' + (', '.join(node['objects']) or '—'))

        def set_details(self, text):
            self.details.configure(state=tk.NORMAL)
            self.details.delete('1.0', tk.END)
            self.details.insert('1.0', text)
            self.details.configure(state=tk.DISABLED)


    class VLSIView(View):
        """Placement of the instances, with zoom (wheel) and pan (drag)."""

        def __init__(self, master):
            super().__init__(master)
            bar = ttk.Frame(self)
            bar.pack(fill=tk.X)
            self.show_names = tk.BooleanVar(value=True)
            self.show_pins = tk.BooleanVar(value=True)
            self.show_nets = tk.BooleanVar(value=True)
            for text, var in (('Noms', self.show_names), ('Broches', self.show_pins), ('Nets', self.show_nets)):
                ttk.Checkbutton(bar, text=text, variable=var, command=self.draw).pack(side=tk.LEFT, padx=4)
            ttk.Button(bar, text='Ajuster', command=self.fit).pack(side=tk.LEFT, padx=(8, 2), pady=2)
            ttk.Button(bar, text='+', width=3, command=lambda: self.zoom_center(1.25)).pack(side=tk.LEFT, padx=2)
            ttk.Button(bar, text='−', width=3, command=lambda: self.zoom_center(1 / 1.25)).pack(side=tk.LEFT, padx=2)
            ttk.Label(bar, text='Zoom : 2 doigts ou +/− · glisser : déplacer',
                      foreground='#666').pack(side=tk.RIGHT, padx=6)
            self.canvas = tk.Canvas(self, background='white', highlightthickness=0, takefocus=True)
            self.canvas.pack(fill=tk.BOTH, expand=True)
            self.canvas.bind('<Configure>', lambda e: self.fit() if self.first else self.draw())
            self.canvas.bind('<ButtonPress-1>', self.press)
            self.canvas.bind('<B1-Motion>', self.drag)
            self.canvas.bind('<MouseWheel>', self.wheel)
            self.canvas.bind('<Button-4>', lambda e: self.zoom(e.x, e.y, 1.2))
            self.canvas.bind('<Button-5>', lambda e: self.zoom(e.x, e.y, 1 / 1.2))
            try:
                # Trackpad of macOS with Tk 9 (two fingers)
                self.canvas.bind('<TouchpadScroll>', self.touchpad)
            except tk.TclError:
                pass
            for key in ('<Key-plus>', '<Key-equal>', '<Key-KP_Add>'):
                self.canvas.bind(key, lambda e: self.zoom_center(1.25))
            for key in ('<Key-minus>', '<Key-KP_Subtract>'):
                self.canvas.bind(key, lambda e: self.zoom_center(1 / 1.25))
            self.canvas.bind('<Key-0>', lambda e: self.fit())
            self.canvas.bind('<Enter>', lambda e: self.canvas.focus_set())
            self.canvas.bind('<Motion>', self.motion)
            self.canvas.bind('<Leave>', lambda e: self.tooltip.hide())
            self.tooltip = Tooltip(self.canvas)
            self.scene = None
            self.scale = 1.0
            self.ox = self.oy = 0.0
            self.first = True
            self.items = {}

        def summary(self, r):
            box = r['bounding_box']
            return ['Circuit : %d × %d' % tuple(r['size']), 'Instances : %d' % r['nb_instances'],
                    'Nets : %d' % r['nb_nets'], '',
                    'Boîte englobante : %d × %d' % (box[2] - box[0], box[3] - box[1]),
                    'Aire relative : %.4f' % r['area_ratio'], 'Longueur des nets : %g' % r['length'],
                    'Fitness : %.4f' % r['fitness'], 'Générations : %d' % r['nb_generations']]

        def show(self, result, problem):
            self.scene = vlsi_scene(problem, result)
            masters = sorted({i['master'] for i in self.scene['instances']})
            self.colors = dict(zip(masters, palette(len(masters))))
            self.fit()

        def bounds(self):
            w, h = self.scene['size']
            x1, y1, x2, y2 = 0, 0, w, h
            for rects in self.scene['board'].values():
                for r in rects:
                    x1, y1, x2, y2 = min(x1, r[0]), min(y1, r[1]), max(x2, r[2]), max(y2, r[3])
            return x1, y1, x2, y2

        def fit(self):
            if not self.scene:
                return
            self.first = False
            cw, ch = max(self.canvas.winfo_width(), 50), max(self.canvas.winfo_height(), 50)
            x1, y1, x2, y2 = self.bounds()
            margin = 20
            self.scale = min((cw - 2 * margin) / max(x2 - x1, 1), (ch - 2 * margin) / max(y2 - y1, 1))
            self.ox = margin - x1 * self.scale + (cw - 2 * margin - (x2 - x1) * self.scale) / 2
            self.oy = ch - margin + y1 * self.scale - (ch - 2 * margin - (y2 - y1) * self.scale) / 2
            self.draw()

        def to_canvas(self, x, y):
            return self.ox + x * self.scale, self.oy - y * self.scale

        def rect(self, r, **kw):
            a = self.to_canvas(r[0], r[1])
            b = self.to_canvas(r[2], r[3])
            return self.canvas.create_rectangle(a[0], b[1], b[0], a[1], **kw)

        def draw(self):
            c = self.canvas
            c.delete('all')
            self.items.clear()
            if not self.scene:
                return
            w, h = self.scene['size']
            self.rect((0, 0, w, h), outline='#999', fill='#f7f7f7', dash=(4, 3))
            for inst in self.scene['instances']:
                pts = [coord for p in inst['shape'] for coord in self.to_canvas(*p)]
                item = c.create_polygon(pts, fill=self.colors[inst['master']], outline='#333')
                self.items[item] = '%s (%s)\n%s à (%d, %d), %d × %d' % (
                    inst['id'], inst['master'], inst['orientation'], inst['position'][0], inst['position'][1],
                    inst['size'][0], inst['size'][1])
                if self.show_pins.get():
                    for term, r in inst['pins']:
                        pin = self.rect(r, fill='#333', outline='')
                        self.items[pin] = '%s.%s' % (inst['id'], term)
            for term, rects in self.scene['board'].items():
                for r in rects:
                    item = self.rect(r, fill='#c01c28', outline='')
                    self.items[item] = 'Borne du circuit : %s' % term
                    if self.show_names.get():
                        x, y = self.to_canvas((r[0] + r[2]) / 2, r[3])
                        c.create_text(x, y - 8, text=term, fill='#c01c28', font=('TkDefaultFont', 8))
            if self.show_nets.get():
                for wire in self.scene['wires']:
                    (x1, y1), (x2, y2) = wire['from'], wire['to']
                    a, m, b = self.to_canvas(x1, y1), self.to_canvas(x2, y1), self.to_canvas(x2, y2)
                    width = 1 + min(wire['weight'], 4) * 0.5
                    item = c.create_line(a[0], a[1], m[0], m[1], b[0], b[1], fill='#2a7ab0', width=width)
                    self.items[item] = 'Net %s (poids %g)' % (wire['net'], wire['weight'])
            if self.scene['box']:
                self.rect(self.scene['box'], outline='#c01c28', dash=(6, 3), width=1.5)
            if self.show_names.get():
                for inst in self.scene['instances']:
                    xs = [p[0] for p in inst['shape']]
                    ys = [p[1] for p in inst['shape']]
                    x, y = self.to_canvas((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2)
                    c.create_text(x, y, text=inst['id'], font=('TkDefaultFont', 9, 'bold'))

        def press(self, event):
            self.canvas.focus_set()
            self.last = (event.x, event.y)

        def drag(self, event):
            self.ox += event.x - self.last[0]
            self.oy += event.y - self.last[1]
            self.last = (event.x, event.y)
            self.draw()

        def wheel(self, event):
            if not event.delta:
                return
            if abs(event.delta) >= 120:
                # Multiples of 120 (Windows, and macOS with Tk 9)
                factor = 1.2 ** (event.delta / 120)
            else:
                # Small deltas (trackpad or mouse of macOS with Tk 8.6)
                factor = 1.04 ** max(-10, min(10, event.delta))
            self.zoom(event.x, event.y, factor)

        def touchpad(self, event):
            # Tk 9: the delta contains the horizontal (high 16 bits) and the
            # vertical (low 16 bits, signed) displacements
            dy = event.delta & 0xFFFF
            if dy >= 0x8000:
                dy -= 0x10000
            if dy:
                self.zoom(event.x, event.y, 1.02 ** max(-15, min(15, -dy)))

        def zoom_center(self, factor):
            self.zoom(self.canvas.winfo_width() / 2, self.canvas.winfo_height() / 2, factor)

        def zoom(self, x, y, factor):
            self.ox = x - (x - self.ox) * factor
            self.oy = y - (y - self.oy) * factor
            self.scale *= factor
            self.draw()

        def motion(self, event):
            for item in reversed(self.canvas.find_overlapping(event.x - 1, event.y - 1, event.x + 1, event.y + 1)):
                if item in self.items:
                    self.tooltip.show(self.items[item], event.x, event.y)
                    return
            self.tooltip.hide()


    class AppTab(ttk.Frame):
        """Tab of an application: parameters, run, result and log."""

        def __init__(self, master, key, view_class, settings):
            super().__init__(master)
            self.key = key
            self.app = APPS[key]
            self.settings = settings
            self.queue = queue.Queue()
            self.process = []
            self.result = None
            self.problem = None
            self.running = False

            # Parameters
            params = ttk.LabelFrame(self, text='Paramètres')
            params.pack(fill=tk.X, padx=6, pady=4)
            saved = settings.get(key, {})
            self.exe = tk.StringVar(value=saved.get('exe') or find_executable(self.app['exe']))
            self.problem_path = tk.StringVar(value=saved.get('problem') or str(self.app['example']))
            self.config_path = tk.StringVar(value=saved.get('config', ''))
            rows = (('Programme', self.exe, self.browse_exe, None),
                    ('Problème', self.problem_path, self.browse_problem, ('Exemple', self.use_example)),
                    ('Configuration', self.config_path, self.browse_config, ('Aucune', lambda: self.config_path.set(''))))
            for row, (label, var, browse, extra) in enumerate(rows):
                ttk.Label(params, text=label).grid(row=row, column=0, sticky=tk.W, padx=4, pady=2)
                ttk.Entry(params, textvariable=var).grid(row=row, column=1, sticky=tk.EW, padx=4, pady=2)
                ttk.Button(params, text='Parcourir…', command=browse).grid(row=row, column=2, padx=2)
                if extra:
                    ttk.Button(params, text=extra[0], command=extra[1]).grid(row=row, column=3, padx=2)
            params.columnconfigure(1, weight=1)

            line = ttk.Frame(params)
            line.grid(row=3, column=0, columnspan=4, sticky=tk.EW, pady=4)
            self.population = tk.StringVar(value='16')
            self.generations = tk.StringVar(value='20')
            self.threads = tk.StringVar()
            self.heuristic = tk.StringVar()
            for label, var, width in (('Population', self.population, 6), ('Générations', self.generations, 6),
                                      ('Threads', self.threads, 4)):
                ttk.Label(line, text=label).pack(side=tk.LEFT, padx=(4, 2))
                ttk.Spinbox(line, textvariable=var, from_=1, to=100000, width=width).pack(side=tk.LEFT, padx=(0, 8))
            ttk.Label(line, text='Heuristique').pack(side=tk.LEFT, padx=(4, 2))
            ttk.Combobox(line, textvariable=self.heuristic, values=[''] + self.app['heuristics'],
                         width=22, state='readonly').pack(side=tk.LEFT)
            ttk.Label(line, text='(vide = valeur du problème ou par défaut)', foreground='#666').pack(side=tk.LEFT, padx=6)

            buttons = ttk.Frame(self)
            buttons.pack(fill=tk.X, padx=6)
            self.run_button = ttk.Button(buttons, text='▶ Lancer', command=self.run)
            self.run_button.pack(side=tk.LEFT, padx=2)
            self.stop_button = ttk.Button(buttons, text='■ Arrêter', command=self.stop, state=tk.DISABLED)
            self.stop_button.pack(side=tk.LEFT, padx=2)
            ttk.Button(buttons, text='Ouvrir un résultat…', command=self.open_result).pack(side=tk.LEFT, padx=12)
            ttk.Button(buttons, text='Enregistrer le résultat…', command=self.save_result).pack(side=tk.LEFT, padx=2)
            self.status = tk.StringVar(value='Prêt.')
            ttk.Label(buttons, textvariable=self.status).pack(side=tk.RIGHT, padx=4)

            # Result: view + summary, and log below
            vertical = ttk.PanedWindow(self, orient=tk.VERTICAL)
            vertical.pack(fill=tk.BOTH, expand=True, padx=6, pady=4)
            horizontal = ttk.PanedWindow(vertical, orient=tk.HORIZONTAL)
            self.view = view_class(horizontal)
            horizontal.add(self.view, weight=5)
            side = ttk.LabelFrame(horizontal, text='Résumé')
            self.summary = tk.Text(side, width=30, wrap=tk.WORD, state=tk.DISABLED, borderwidth=0,
                                   background=self.winfo_toplevel().cget('background'))
            self.summary.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)
            horizontal.add(side, weight=1)
            vertical.add(horizontal, weight=5)
            logframe = ttk.LabelFrame(vertical, text='Journal')
            self.log = tk.Text(logframe, height=7, wrap=tk.NONE, font=('TkFixedFont', 9))
            scroll = ttk.Scrollbar(logframe, orient=tk.VERTICAL, command=self.log.yview)
            self.log.configure(yscrollcommand=scroll.set)
            self.log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            scroll.pack(side=tk.RIGHT, fill=tk.Y)
            vertical.add(logframe, weight=1)

        # Files
        def browse_exe(self):
            path = filedialog.askopenfilename(title='Programme ' + self.app['exe'])
            if path:
                self.exe.set(path)

        def browse_problem(self):
            path = filedialog.askopenfilename(title='Problème', filetypes=[('JSON', '*.json'), ('Tous', '*')],
                                              initialdir=str(Path(self.problem_path.get() or ROOT).parent))
            if path:
                self.problem_path.set(path)

        def browse_config(self):
            path = filedialog.askopenfilename(title='Configuration', filetypes=[('JSON', '*.json'), ('Tous', '*')])
            if path:
                self.config_path.set(path)

        def use_example(self):
            self.problem_path.set(str(self.app['example']))

        def load_problem(self):
            """Read the problem (JSON) if possible (it is needed by some views)."""
            try:
                return json.loads(Path(self.problem_path.get()).read_text(encoding='utf-8'))
            except (OSError, ValueError):
                return None

        # Run
        def config(self):
            config = {}
            if self.config_path.get():
                config = json.loads(Path(self.config_path.get()).read_text(encoding='utf-8'))
            override = make_config(self.population.get(), self.generations.get(), self.threads.get(),
                                   self.heuristic.get(), self.app['heuristic'])
            for k, v in override.items():
                if isinstance(v, dict) and isinstance(config.get(k), dict):
                    config[k].update(v)
                else:
                    config[k] = v
            return config

        def run(self):
            exe = self.exe.get()
            if not exe or not Path(exe).exists():
                messagebox.showerror('evocomb', "Le programme '%s' est introuvable.\nChoisissez-le avec « Parcourir… »." % (exe or self.app['exe']))
                return
            if not Path(self.problem_path.get()).exists():
                messagebox.showerror('evocomb', "Le problème '%s' est introuvable." % self.problem_path.get())
                return
            try:
                config = self.config()
            except (OSError, ValueError) as e:
                messagebox.showerror('evocomb', 'Configuration invalide : %s' % e)
                return
            self.save_settings()
            self.process.clear()
            self.running = True
            self.start = time.time()
            self.run_button.configure(state=tk.DISABLED)
            self.stop_button.configure(state=tk.NORMAL)
            self.set_log('$ %s %s\n' % (exe, self.problem_path.get()) + ('config : %s\n' % json.dumps(config) if config else ''))
            problem = self.problem_path.get()

            def work():
                try:
                    self.queue.put(run_program(exe, problem, config, self.process))
                except Exception as e:      # Reported in the interface
                    self.queue.put({'code': -1, 'result': None, 'stderr': str(e), 'log': '', 'time': 0})

            threading.Thread(target=work, daemon=True).start()
            self.poll()

        def poll(self):
            try:
                out = self.queue.get_nowait()
            except queue.Empty:
                self.status.set('En cours… %.0f s' % (time.time() - self.start))
                self.after(200, self.poll)
                return
            self.running = False
            self.run_button.configure(state=tk.NORMAL)
            self.stop_button.configure(state=tk.DISABLED)
            self.append_log(out['stderr'] + out['log'])
            if out['result'] is None:
                self.status.set('Échec (code %s).' % out['code'])
                return
            self.status.set('Terminé en %.1f s.' % out['time'])
            self.display(out['result'])

        def stop(self):
            for proc in self.process:
                if proc.poll() is None:
                    proc.terminate()

        # Results
        def display(self, result):
            self.result = result
            try:
                self.view.show(result, self.load_problem())
                lines = self.view.summary(result)
            except (KeyError, TypeError, ValueError) as e:
                messagebox.showerror('evocomb', "Ce résultat ne correspond pas à l'application « %s » (%s)." % (self.app['title'], e))
                return
            self.summary.configure(state=tk.NORMAL)
            self.summary.delete('1.0', tk.END)
            self.summary.insert('1.0', '\n'.join(lines))
            self.summary.configure(state=tk.DISABLED)

        def open_result(self):
            path = filedialog.askopenfilename(title='Résultat', filetypes=[('JSON', '*.json'), ('Tous', '*')])
            if not path:
                return
            try:
                result = json.loads(Path(path).read_text(encoding='utf-8'))
            except (OSError, ValueError) as e:
                messagebox.showerror('evocomb', 'Impossible de lire le résultat : %s' % e)
                return
            self.status.set('Résultat chargé : %s' % Path(path).name)
            self.display(result)

        def save_result(self):
            if not self.result:
                messagebox.showinfo('evocomb', 'Aucun résultat à enregistrer.')
                return
            path = filedialog.asksaveasfilename(title='Enregistrer le résultat', defaultextension='.json',
                                                filetypes=[('JSON', '*.json')])
            if path:
                Path(path).write_text(json.dumps(self.result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')

        # Log
        def set_log(self, text):
            self.log.delete('1.0', tk.END)
            self.log.insert(tk.END, text)

        def append_log(self, text):
            self.log.insert(tk.END, text)
            self.log.see(tk.END)

        # Settings
        def save_settings(self):
            self.settings[self.key] = {'exe': self.exe.get(), 'problem': self.problem_path.get(),
                                       'config': self.config_path.get()}
            try:
                SETTINGS.write_text(json.dumps(self.settings, indent=2), encoding='utf-8')
            except OSError:
                pass


    class MainWindow(tk.Tk):
        """Main window: one tab per application."""

        def __init__(self):
            super().__init__()
            self.title('evocomb — EvoComb')
            self.geometry('1150x780')
            try:
                settings = json.loads(SETTINGS.read_text(encoding='utf-8'))
            except (OSError, ValueError):
                settings = {}
            style = ttk.Style(self)
            if 'clam' in style.theme_names() and sys.platform.startswith('linux'):
                style.theme_use('clam')
            notebook = ttk.Notebook(self)
            notebook.pack(fill=tk.BOTH, expand=True)
            self.tabs = {}
            for key, view in (('bin_packing', BinPackingView), ('hierarchy', HierarchyView), ('vlsi', VLSIView)):
                tab = AppTab(notebook, key, view, settings)
                notebook.add(tab, text=APPS[key]['title'])
                self.tabs[key] = tab


def main():
    if tk is None:
        sys.exit('tkinter is not available (on Linux, install the package python3-tk).')
    MainWindow().mainloop()


if __name__ == '__main__':
    main()
