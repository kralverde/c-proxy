import urllib.request
import asyncio

with open('test.txt', 'r') as f:
    data = ' '.join(x.strip() for x in f.read().split('\n'))

async def run():
    fp = urllib.request.urlopen("http://127.0.0.1:8080")
    mybytes = fp.read()

    mystr = mybytes.decode("utf8")
    fp.close()

    assert data == mystr

async def multi():
    routines = [run()] * 100
    res = await asyncio.gather(*routines, return_exceptions=True)
    return res

if __name__ == '__main__':
    e = asyncio.get_event_loop().run_until_complete(multi())
    print(not any(e))
    if any(e):
        print(e)