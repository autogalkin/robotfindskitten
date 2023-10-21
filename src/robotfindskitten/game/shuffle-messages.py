from random import shuffle

with open('messages.txt', 'r') as f:
    with open('messages-out.txt', 'w') as w:
        msgs = [ i for i in [i.strip() for i in f.readlines()] if i]
        shuffle(msgs)
        w.write('\n\n'.join(msgs))
