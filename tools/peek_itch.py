import gzip, struct, collections, sys

path = sys.argv[1] if len(sys.argv) > 1 else "data/20191230.BX_ITCH_50.gz"
counts = collections.Counter()

with gzip.open(path, "rb") as f:
    for i in range(200000):
        hdr = f.read(2)
        if len(hdr) < 2:
            break
        (n,) = struct.unpack(">H", hdr)     # BinaryFILE: 2-byte big-endian length
        body = f.read(n)
        t = chr(body[0])
        counts[t] += 1
        if i < 10:
            print(f"{i:3d}  len={n:3d}  type={t}")

print("\nmessage type counts (first 200k):")
for t, c in counts.most_common():
    print(f"  {t}  {c}")
