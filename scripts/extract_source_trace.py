#!/usr/bin/env python3
"""Extract an auditable identifier-token trace from C/C++ source files.

This is a lexical trace, not a full C++ parser: scope depth is derived from braces
after comments and string/character literals are masked. Point it at an actual
compiler/source tree for a project-specific trace.
"""
import argparse, csv, re
from pathlib import Path

KEYWORDS = set("""alignas alignof and and_eq asm auto bitand bitor bool break case catch char
char8_t char16_t char32_t class compl concept const consteval constexpr constinit const_cast
continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum
explicit export extern false float for friend goto if inline int long mutable namespace new
noexcept not not_eq nullptr operator or or_eq private protected public register reinterpret_cast
requires return short signed sizeof static static_assert static_cast struct switch template this
thread_local throw true try typedef typeid typename union unsigned using virtual void volatile
wchar_t while xor xor_eq""".split())
TOKEN_RE = re.compile(r'[A-Za-z_][A-Za-z_0-9]*|[{}]')
COMMENT_STRING_RE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)

def mask_noncode(text):
    def repl(m):
        return ''.join('\n' if c == '\n' else ' ' for c in m.group(0))
    return COMMENT_STRING_RE.sub(repl, text)

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--root", default=".", help="Directory containing C/C++ source")
    ap.add_argument("--output", default="data/source_identifier_trace.csv")
    ap.add_argument("--extensions", default=".c,.h,.cc,.cpp,.hpp,.cxx")
    args=ap.parse_args()
    root=Path(args.root).resolve()
    exts=set(args.extensions.split(","))
    files=sorted(p for p in root.rglob("*") if p.is_file() and p.suffix in exts and ".git" not in p.parts and "build" not in p.parts)
    out=Path(args.output)
    out.parent.mkdir(parents=True,exist_ok=True)
    rows=[]
    for path in files:
        try: text=mask_noncode(path.read_text(encoding="utf-8", errors="ignore"))
        except OSError: continue
        depth=0
        # Process line by line so line numbers remain meaningful.
        for lineno,line in enumerate(text.splitlines(),1):
            for m in TOKEN_RE.finditer(line):
                token=m.group(0)
                if token == "{":
                    depth += 1
                elif token == "}":
                    depth=max(0,depth-1)
                elif token[0].isalpha() or token[0]=="_":
                    rows.append({
                        "SourceFile": str(path.relative_to(root)),
                        "Line": lineno,
                        "Identifier": token,
                        "ScopeDepthApprox": depth,
                        "IsKeyword": int(token in KEYWORDS),
                    })
    with out.open("w", newline="", encoding="utf-8") as f:
        w=csv.DictWriter(f,fieldnames=["SourceFile","Line","Identifier","ScopeDepthApprox","IsKeyword"])
        w.writeheader(); w.writerows(rows)
    unique=len({r["Identifier"] for r in rows if not r["IsKeyword"]})
    print(f"Scanned {len(files)} source files; wrote {len(rows)} identifier/keyword tokens; {unique} unique non-keyword identifiers to {out}")
    print("Note: brace depth is an approximation, not compiler-grade scope parsing.")

if __name__=="__main__": main()
