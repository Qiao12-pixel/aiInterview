"""Small independent protobuf/gRPC-Web client for integration tests, stdlib only."""
import http.client
import json
import struct
from urllib.parse import unquote

def varint(n):
    out = bytearray()
    while n > 127:
        out.append((n & 127) | 128)
        n >>= 7
    out.append(n)
    return out

def message(fields):
    out = bytearray()
    for key, value in fields.items():
        if isinstance(value, int):
            out += varint(key * 8) + varint(value)
        else:
            data = value.encode() if isinstance(value, str) else value
            out += varint(key * 8 + 2) + varint(len(data)) + data
    return bytes(out)

def decode(data):
    offset = 0
    def read():
        nonlocal offset
        n, shift = 0, 0
        while True:
            b = data[offset]
            offset += 1
            n |= (b & 127) << shift
            if b < 128:
                return n
            shift += 7
    out = {}
    while offset < len(data):
        tag = read()
        if tag % 8 == 0:
            value = read()
        elif tag % 8 == 2:
            size = read()
            value = data[offset:offset+size]
            offset += size
        else:
            raise ValueError('unsupported protobuf wire')
        out[tag // 8] = value
    return out

class Client:
    def __init__(self, port, cookie=''):
        self.port, self.cookie = port, cookie
    def call(self, method, fields):
        data = message(fields)
        conn = http.client.HTTPConnection('127.0.0.1', self.port, timeout=30)
        try:
            conn.request('POST', '/aiinterview.v1.InterviewService/' + method,
                         b'\0'+struct.pack('!I',len(data))+data,
                         {'Content-Type':'application/grpc-web+proto','Cookie':self.cookie})
            res = conn.getresponse()
            cookie = res.getheader('Set-Cookie')
            if cookie:
                self.cookie = cookie.split(';')[0]
            body = res.read()
            if res.status != 200:
                raise RuntimeError('HTTP '+str(res.status))
            frames, terminal = [], False
            while body:
                flag, length = body[0], struct.unpack('!I',body[1:5])[0]
                value, body = body[5:5+length], body[5+length:]
                if len(value)!=length:
                    raise ValueError('truncated frame')
                if flag == 128:
                    trailers = dict((k.strip(),v.strip()) for k,v in (line.split(':',1) for line in value.decode().strip().split('\r\n')))
                    if trailers['grpc-status']!='0':
                        raise RuntimeError('gRPC '+trailers['grpc-status']+': '+unquote(trailers.get('grpc-message','')))
                    terminal = True
                elif flag == 0:
                    frames.append(decode(value))
                else:
                    raise ValueError('invalid frame flag')
            if not terminal:
                raise ValueError('missing trailers')
            return frames
        finally:
            conn.close()
    def account(self, operation, body=None):
        return json.loads(self.call('Account',{1:operation,2:json.dumps(body or {})})[0][1])
    def cycle(self):
        events=self.call('StartInterview',{1:'Synthetic load',3:1})
        assert any(e[1]==7 for e in events), 'missing question delta'
        sid=next(e[2].decode() for e in events if e[1]==8)
        events=self.call('SendText',{1:sid,2:'RAII binds resource lifetime to object lifetime.'})
        assert any(e[1]==5 for e in events),'missing summary'
        report=json.loads(self.call('GetReport',{1:sid})[0][2])
        assert report['session_id']==sid
        return sid
