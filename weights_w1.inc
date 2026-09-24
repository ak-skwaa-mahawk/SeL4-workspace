mkdir -p ~/seL4-workspace/apps/rootserver/src/tinyml
python3 -c '
w1 = ", ".join(["{" + ", ".join(["1"] * 128) + "}"] * 16)
b1 = ", ".join(["0"] * 16)
w2 = ", ".join(["{" + ", ".join(["1"] * 16) + "}"] * 2)
b2 = ", ".join(["0"] * 2)

with open("/data/data/com.termux/files/home/seL4-workspace/apps/rootserver/src/tinyml/weights_w1.inc", "w") as f: f.write(w1)
with open("/data/data/com.termux/files/home/seL4-workspace/apps/rootserver/src/tinyml/biases_b1.inc", "w") as f: f.write(b1)
with open("/data/data/com.termux/files/home/seL4-workspace/apps/rootserver/src/tinyml/weights_w2.inc", "w") as f: f.write(w2)
with open("/data/data/com.termux/files/home/seL4-workspace/apps/rootserver/src/tinyml/biases_b2.inc", "w") as f: f.write(b2)
'
