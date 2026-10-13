"""Read-only source audit. Outputs lexical evidence, not a C++ compiler call graph.
Run with bundled Python: audit_structure.py [repository root].
Only writes generated evidence beside this script; never edits source inputs.
"""
from pathlib import Path
from collections import Counter, defaultdict
import bisect
import csv
import hashlib
import json
import re
import sys
import xml.etree.ElementTree as ET

ROOT = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
SOURCES = sorted(p for base in ('WiNILL', 'tests') for p in (ROOT / base).rglob('*')
                 if p.suffix in ('.cpp', '.h', '.inl') and p.name != 'miniaudio.h')
INCLUDE_DIRS = [ROOT / 'WiNILL' / p for p in ('Core', 'Core/Scenes', 'Render', 'Entity', 'System')]
INCLUDE_DIRS += [ROOT / 'Include']
TOKEN = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|(?:u8|[LuU])?"(?:\\.|[^"\\])*"|'
                   r"(?:[LuU])?'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|"
                   r'\d+(?:\.\d*)?(?:[eE][+-]?\d+)?[fFuUlL]*|::|->|==|!=|<=|>=|&&|\|\||\+\+|--|.' )
CONTROL = {'if', 'for', 'while', 'switch', 'catch', 'sizeof', 'alignof', 'decltype', 'noexcept'}
KEYWORDS = CONTROL | set('return const static inline constexpr auto void bool float double int long short unsigned signed char wchar_t struct class enum public private protected true false nullptr new delete break continue else case default using namespace template typename virtual override final extern volatile mutable friend std'.split())

def relative(p):
    return p.resolve().relative_to(ROOT).as_posix()

def table(name, header, rows):
    with (OUT / name).open('w', encoding='utf-8-sig', newline='') as f:
        w = csv.writer(f, delimiter='\t'); w.writerow(header); w.writerows(rows)

def tokenize(text):
    # Keep offsets/line numbers while ignoring all preprocessor directives.
    text = re.sub(r'(?m)^[ \t]*#[^\n]*(?:\\\n[^\n]*)*',
                  lambda m: ''.join('\n' if c == '\n' else ' ' for c in m.group()), text)
    starts = [0] + [m.end() for m in re.finditer('\n', text)]
    result = []
    for m in TOKEN.finditer(text):
        t = m.group()
        if t.isspace() or t.startswith('//') or t.startswith('/*'): continue
        result.append((t, bisect.bisect_right(starts, m.start()), m.start(), m.end()))
    return result

def functions(path, toks):
    pairs = {}; stack = []
    for i, (t, *_) in enumerate(toks):
        if t in ('(', '{', '['): stack.append((t, i))
        elif t in (')', '}', ']') and stack:
            wanted = {')':'(', '}':'{', ']':'['}[t]
            if stack[-1][0] == wanted:
                _, begin = stack.pop(); pairs[begin] = i; pairs[i] = begin
    found = []
    for i, (t, line, *_rest) in enumerate(toks):
        if t != '{' or i not in pairs: continue
        j = i - 1
        while j >= 0 and toks[j][0] in ('const', 'override', 'final', 'noexcept'): j -= 1
        if j < 0 or toks[j][0] != ')' or j not in pairs: continue
        op = pairs[j]; ni = op - 1
        if ni < 0 or not re.fullmatch(r'[A-Za-z_]\w*', toks[ni][0]): continue
        name = toks[ni][0]
        if name in CONTROL or (ni and toks[ni-1][0] in ('.', '->', '=', ',', ':')): continue
        # Excludes braced call initializers; constructors with initializer lists
        # and operator overloads are deliberately not guaranteed by this lexer.
        start = ni
        while start > 0 and toks[start-1][0] not in (';', '{', '}'): start -= 1
        prefix = [x[0] for x in toks[start:ni]]
        if '=' in prefix or 'return' in prefix: continue
        end = pairs[i]
        found.append({'file':path, 'name':name, 'line':toks[start][1], 'body_line':line,
                      'end':toks[end][1], 'token_start':i, 'token_end':end,
                      'signature':' '.join(x[0] for x in toks[start:i])[:260]})
    return found

files = {}; func_list = []; includes = []; global_refs = defaultdict(dict)
for p in SOURCES:
    raw = p.read_bytes(); text = raw.decode('utf-8-sig', errors='replace'); path = relative(p)
    toks = tokenize(text); funcs = functions(path, toks); func_list.extend(funcs)
    files[path] = {'path':path, 'lines':len(text.splitlines()), 'bytes':len(raw),
                   'sha256':hashlib.sha256(raw).hexdigest(), 'replacement_chars':text.count('\ufffd'),
                   'tokens':toks, 'text':text, 'functions':funcs}
    refs = defaultdict(list)
    for t, line, *_ in toks:
        if re.fullmatch(r'g_\w+', t): refs[t].append(line)
    for symbol, lines in refs.items(): global_refs[symbol][path] = sorted(set(lines))
    for m in re.finditer(r'^\s*#\s*include\s*([<"])([^>"\n]+)[>"]', text, re.M):
        local = m.group(1) == '"'; inc = m.group(2).replace('\\', '/')
        candidates = ([p.parent / inc] if local else []) + [base / inc for base in INCLUDE_DIRS]
        resolved = next((c.resolve() for c in candidates if c.is_file()), None)
        target = relative(resolved) if resolved and resolved.is_relative_to(ROOT) else ''
        includes.append((path, text.count('\n',0,m.start())+1, inc, target,
                         'owned' if target in files or target.startswith(('WiNILL/', 'tests/')) and not target.endswith('miniaudio.h') else 'external',
                         'quoted' if local else 'system'))

# Resolve classification after all files have been indexed.
includes = [(s,l,i,t,'owned' if t in files else 'external',k) for s,l,i,t,_,k in includes]
adj = {p:set() for p in files}; reverse = {p:set() for p in files}
for s,l,i,t,kind,k in includes:
    if kind == 'owned': adj[s].add(t); reverse[t].add(s)

def closure(start, graph):
    seen = set(); todo = list(graph[start])
    while todo:
        x=todo.pop()
        if x in seen: continue
        seen.add(x); todo.extend(graph[x]-seen)
    return seen

# Strongly connected components (all preprocessor branches considered).
index = {}; low = {}; stack = []; on = set(); components = []
def visit(v):
    index[v] = low[v] = len(index); stack.append(v); on.add(v)
    for w in adj[v]:
        if w not in index: visit(w); low[v] = min(low[v],low[w])
        elif w in on: low[v] = min(low[v],index[w])
    if low[v] == index[v]:
        comp=[]
        while True:
            w=stack.pop(); on.remove(w); comp.append(w)
            if w==v: break
        components.append(sorted(comp))
for p in files:
    if p not in index: visit(p)

table('files.tsv', ['file','lines','bytes','sha256','UTF8_replacement_chars','direct_owned_dependencies','direct_dependents','transitive_owned_dependencies','transitive_dependents','lexical_function_definitions'],
      [(p,v['lines'],v['bytes'],v['sha256'],v['replacement_chars'],len(adj[p]),len(reverse[p]),len(closure(p,adj)),len(closure(p,reverse)),len(v['functions'])) for p,v in files.items()])
table('includes.tsv',['source','line','include_text','resolved_target','kind','syntax'], includes)
table('functions.tsv',['file','name','start_line','body_line','end_line','signature'],
      [(f['file'],f['name'],f['line'],f['body_line'],f['end'],f['signature']) for f in func_list])
table('global_references.tsv',['symbol','file','reference_lines','reference_line_count'],
      [(s,p,','.join(map(str,lines)),len(lines)) for s,paths in sorted(global_refs.items()) for p,lines in sorted(paths.items())])

# Syntactic call sites: no overload resolution, macro expansion or indirect calls.
symbols = {f['name'] for f in func_list}; calls=[]
for path,v in files.items():
    toks=v['tokens']; defs={(f['name'],f['body_line']) for f in v['functions']}
    for i in range(len(toks)-1):
        name,line,*_=toks[i]
        if name not in symbols or toks[i+1][0]!='(': continue
        containing=[f for f in v['functions'] if f['token_start']<i<f['token_end']]
        if not containing: continue
        caller=min(containing,key=lambda f:f['token_end']-f['token_start'])
        # A nested definition's signature isn't a call.
        if any(f['line']<=line<=f['body_line'] for f in v['functions'] if f is not caller): continue
        calls.append((path,line,caller['name'],caller['line'],name))
table('call_sites.tsv',['file','line','enclosing_function','function_start','callee_name_unresolved'],calls)

# Exact and identifier-normalized 9-statement-line windows; overlapping windows
# with the same occurrence set are combined below. These are review candidates.
clone_sets={}
for mode in ('exact','renamed'):
    windows=defaultdict(list)
    for path,v in files.items():
        byline=defaultdict(list)
        for t,line,*_ in v['tokens']: byline[line].append(t)
        seq=[(n,ts) for n,ts in sorted(byline.items()) if ''.join(ts) not in ('{','}','};') and ts[0]!='#']
        for i in range(len(seq)-8):
            chunk=seq[i:i+9]; raw=[t for _,ts in chunk for t in ts]
            if len(raw)<55: continue
            if mode=='renamed':
                names={}; normalized=[]
                for t in raw:
                    if re.fullmatch(r'[A-Za-z_]\w*',t) and t not in KEYWORDS:
                        if t not in names:names[t]=f'ID{len(names)}'
                        t=names[t]
                    normalized.append(t)
                key=tuple(normalized)
            else: key=tuple(raw)
            windows[key].append((path,chunk[0][0],chunk[-1][0]))
    groups=[]
    for key,locs in windows.items():
        if len(locs)<2:continue
        # Ignore overlapping occurrences within a file (e.g. repeated table rows).
        keep=[]
        for loc in locs:
            if not any(p==loc[0] and abs(s-loc[1])<9 for p,s,e in keep):keep.append(loc)
        if len(keep)>1:groups.append((keep,len(key)))
    merged=[]
    for locs,size in sorted(groups,key=lambda g:(tuple(x[0] for x in g[0]),g[0][0][1])):
        previous=next((g for g in reversed(merged[-40:]) if len(g[0])==len(locs) and all(a[0]==b[0] and b[1]<=a[2]+2 and b[1]>=a[1] for a,b in zip(g[0],locs))),None)
        if previous:
            previous[0][:]=[(a[0],a[1],max(a[2],b[2])) for a,b in zip(previous[0],locs)]
            previous[1]=max(previous[1],size)
        else: merged.append([locs,size])
    clone_sets[mode]=merged
    table('clones_'+mode+'.tsv',['candidate','file','start_line','end_line','window_tokens','occurrences'],
          [(f'{mode}-{i:03}',p,s,e,size,len(locs)) for i,(locs,size) in enumerate(merged,1) for p,s,e in locs])

modules=lambda p:'tests' if p.startswith('tests/') else 'main' if p=='WiNILL/main.cpp' else '/'.join(p.split('/')[1:-1]) or 'resource'
module_edges=Counter((modules(s),modules(t)) for s,l,i,t,k,syntax in includes if k=='owned' and modules(s)!=modules(t))
table('module_edges.tsv',['source_module','dependency_module','include_directive_count'],[(s,t,n) for (s,t),n in sorted(module_edges.items())])

ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
vc=ET.parse(ROOT/'WiNILL/WiNILL.vcxproj')
vc_sources={relative(ROOT/'WiNILL'/e.attrib['Include'].replace('\\','/')) for e in vc.findall('.//m:ClCompile[@Include]',ns)}
cmake=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
cm_sources={m.replace('${WINILL_DIR}','WiNILL').replace('${CMAKE_SOURCE_DIR}/','') for m in re.findall(r'\$\{(?:WINILL_DIR|CMAKE_SOURCE_DIR)\}/[^\s)]+\.(?:cpp|c)',cmake)}
owned_cpp={p for p in files if p.startswith('WiNILL/') and p.endswith('.cpp')}
support=sorted(set([ROOT/'CMakeLists.txt',ROOT/'WiNILL.sln',ROOT/'glad.c',ROOT/'WiNILL/Resource.rc',ROOT/'WiNILL/WiNILL.vcxproj',ROOT/'WiNILL/WiNILL.vcxproj.filters']+[p for base in ('scripts','.github') for p in (ROOT/base).rglob('*') if p.is_file()]))
table('support_files.tsv',['file','lines','sha256','classification'],[(relative(p),len(p.read_bytes().splitlines()),hashlib.sha256(p.read_bytes()).hexdigest(),'generated_GLAD_external' if p.name=='glad.c' else 'build_resource_script') for p in support])
summary={'source_files':len(files),'game_source_files':sum(p.startswith('WiNILL/') for p in files),'test_source_files':sum(p.startswith('tests/') for p in files),
         'game_lines':sum(v['lines'] for p,v in files.items() if p.startswith('WiNILL/')),'test_lines':sum(v['lines'] for p,v in files.items() if p.startswith('tests/')),
         'include_directives':len(includes),'owned_include_directives':sum(k=='owned' for s,l,i,t,k,syntax in includes),'unique_owned_include_edges':sum(map(len,adj.values())),
         'lexical_functions':len(func_list),'syntactic_call_sites':len(calls),'distinct_g_symbols':len(global_refs),
         'cycles':[c for c in components if len(c)>1],
         'exact_clone_candidates':len(clone_sets['exact']),'renamed_clone_candidates':len(clone_sets['renamed']),
         'cpp_missing_from_vs':sorted(owned_cpp-vc_sources),'cpp_missing_from_cmake':sorted(owned_cpp-cm_sources),
         'vs_only_sources':sorted(vc_sources-cm_sources),'cmake_only_sources':sorted(cm_sources-vc_sources),
         'quoted_unresolved':[(s,l,i) for s,l,i,t,k,syntax in includes if syntax=='quoted' and not t],
         'top_dependency_hubs':sorted([(p,len(reverse[p]),len(closure(p,reverse))) for p in files],key=lambda x:(x[2],x[1]),reverse=True)[:20],
         'support_files':len(support)}
(OUT/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
manifest={p:{k:v[k] for k in ('sha256','lines')} for p,v in files.items()}
(OUT/'baseline_hashes.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
assert len(manifest)==len(SOURCES)
assert all(s in files and (k!='owned' or t in files) for s,l,i,t,k,syntax in includes)
assert len({f'{f["file"]}:{f["body_line"]}' for f in func_list})==len(func_list)
print(json.dumps(summary,ensure_ascii=False,indent=2))
