#!/usr/bin/env python3
"""C++ 网关契约回归：仅 Python 标准库 + 本机 mysql/redis-cli，不依赖 Node.js。"""
import argparse
import base64
import hashlib
import http.client
import json
import os
from pathlib import Path
import secrets
import selectors
import socket
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]


def port():
    with socket.socket() as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


def sql(statement):
    return subprocess.run(['mysql', '-h', '127.0.0.1', '-u', 'root', '-N'], input=statement,
                          text=True, check=True, capture_output=True).stdout


class Processes:
    def __init__(self):
        self.items = []

    def start(self, args):
        child = subprocess.Popen([str(a) for a in args], cwd=ROOT, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.items.append(child)
        selector = selectors.DefaultSelector()
        selector.register(child.stdout, selectors.EVENT_READ)
        lines = []
        try:
            until = time.monotonic() + 12
            while time.monotonic() < until:
                if selector.select(0.2):
                    line = child.stdout.readline()
                    lines.append(line)
                    if 'listening on' in line or 'Web/WS gateway:' in line:
                        return child
                if child.poll() is not None:
                    raise AssertionError('Startup failed: ' + ''.join(lines))
            raise AssertionError('Startup timeout: ' + ''.join(lines))
        finally:
            selector.close()

    def stop(self, child):
        if child.poll() is None:
            child.terminate()
            try:
                child.wait(timeout=8)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
                raise AssertionError('进程退出超时')
        assert child.returncode in (0, -15), child.stdout.read()

    def close(self):
        for child in reversed(self.items):
            if child.poll() is None:
                child.terminate()
                try:
                    child.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait()
            child.stdout.close()


class Client:
    def __init__(self, http_port):
        self.port = http_port
        self.origin = f'http://127.0.0.1:{http_port}'

    def request(self, path, body=None, cookie='', origin=None, sse=False, method='POST', raw=None):
        connection = http.client.HTTPConnection('127.0.0.1', self.port, timeout=20)
        headers = {'Origin': origin or self.origin, 'Content-Type': 'application/json'}
        if cookie:
            headers['Cookie'] = cookie
        if sse:
            headers['Accept'] = 'text/event-stream'
        data = raw if raw is not None else json.dumps(body or {}, ensure_ascii=False).encode()
        try:
            connection.request(method, path, data if method == 'POST' else None, headers)
            response = connection.getresponse()
            status, response_headers = response.status, dict(response.getheaders())
            text = response.read().decode()
            return status, response_headers, text
        finally:
            connection.close()

    def post(self, path, body=None, cookie='', expected=200):
        status, headers, text = self.request(path, body, cookie)
        assert status == expected, (path, status, text)
        return json.loads(text), headers

    def events(self, path, body, cookie, expected=200):
        status, headers, text = self.request(path, body, cookie, sse=True)
        assert status == expected, (path, status, text)
        assert 'text/event-stream' in headers.get('Content-Type', '')
        events = [json.loads(line[5:]) for line in text.splitlines() if line.startswith('data:')]
        return events


class WebSocket:
    def __init__(self, client, cookie='', origin=None, path='/ws'):
        self.socket = socket.create_connection(('127.0.0.1', client.port), timeout=10)
        self.buffer = b''
        key = base64.b64encode(secrets.token_bytes(16)).decode()
        request = (f'GET {path} HTTP/1.1\r\nHost: 127.0.0.1:{client.port}\r\n'
                   f'Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\n'
                   f'Sec-WebSocket-Key: {key}\r\nOrigin: {origin or client.origin}\r\n'
                   f'Cookie: {cookie}\r\n\r\n')
        self.socket.sendall(request.encode())
        while b'\r\n\r\n' not in self.buffer:
            block = self.socket.recv(4096)
            assert block, 'websocket handshake closed'
            self.buffer += block
        header, self.buffer = self.buffer.split(b'\r\n\r\n', 1)
        if not header.startswith(b'HTTP/1.1 101'):
            self.close()
            raise PermissionError(header.decode())
        expected = base64.b64encode(hashlib.sha1((key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest())
        assert expected in header

    def exact(self, size):
        while len(self.buffer) < size:
            data = self.socket.recv(max(4096, size-len(self.buffer)))
            if not data:
                raise EOFError('websocket closed')
            self.buffer += data
        data, self.buffer = self.buffer[:size], self.buffer[size:]
        return data

    def send(self, payload, opcode=1):
        if not isinstance(payload, bytes):
            payload = json.dumps(payload, ensure_ascii=False).encode()
        mask = secrets.token_bytes(4)
        length = len(payload)
        prefix = bytes([0x80 | opcode, 0x80 | (length if length < 126 else 126 if length < 65536 else 127)])
        if length >= 65536:
            prefix += struct.pack('!Q', length)
        elif length >= 126:
            prefix += struct.pack('!H', length)
        self.socket.sendall(prefix + mask + bytes(b ^ mask[i % 4] for i, b in enumerate(payload)))

    def receive(self):
        a, b = self.exact(2)
        size = b & 127
        if size == 126:
            size = struct.unpack('!H', self.exact(2))[0]
        elif size == 127:
            size = struct.unpack('!Q', self.exact(8))[0]
        data = self.exact(size)
        if a & 15 == 9:
            self.send(data, 10)
            return self.receive()
        assert a & 15 == 1, 'expected a text frame'
        return json.loads(data)

    def close(self):
        self.socket.close()


def run(args):
    name = 'aiinterview_test_' + secrets.token_hex(6)
    prefix = name + ':'
    processes = Processes()
    try:
        with tempfile.TemporaryDirectory(prefix='aiinterview-gateway-') as directory:
            config = Path(directory)/'config.json'
            config.write_text(json.dumps({'storage': {'enabled': True,
                'mysql': {'host': '127.0.0.1', 'user': 'root', 'database': name},
                'redis': {'host': '127.0.0.1', 'port': 6379, 'prefix': prefix, 'ttl_seconds': 10}}}))
            grpc_port, http_port = port(), port()
            backend_args = [args.backend, '--mock-model', '--config', config, '--listen', f'127.0.0.1:{grpc_port}']
            gateway_args = [args.gateway, '--port', str(http_port), '--grpc-target', f'127.0.0.1:{grpc_port}']
            # 业务后端在完全不存在的独立库上也能首次初始化。
            backend_name=name+'_backend'
            backend_config=Path(directory)/'backend-only.json'
            data=json.loads(config.read_text());data['storage']['mysql']['database']=backend_name
            backend_config.write_text(json.dumps(data))
            try:
                first_backend=processes.start([args.backend,'--mock-model','--config',backend_config,'--listen',f'127.0.0.1:{port()}'])
                processes.stop(first_backend)
                assert sql(f"SELECT COUNT(*) FROM `{backend_name}`.users;").strip()=='0'
                assert sql(f"SELECT COUNT(*) FROM `{backend_name}`.interview_reports;").strip()=='0'
            finally:
                sql(f'DROP DATABASE IF EXISTS `{backend_name}`;')
            # 网关无需数据库/后端凭据即可启动，不应创建任何数据库。
            initial_gateway=processes.start(gateway_args)
            assert Client(http_port).request('/',method='GET')[0]==200
            processes.stop(initial_gateway)
            assert sql(f"SELECT COUNT(*) FROM information_schema.SCHEMATA WHERE SCHEMA_NAME='{name}';").strip()=='0'
            sql(f'CREATE DATABASE `{name}` CHARACTER SET utf8mb4;')
            # 模拟已有旧表并保留历史数据，之后启动自动补列/索引。
            sql(f"DROP TABLE IF EXISTS `{name}`.interview_reports;")
            sql(f"CREATE TABLE `{name}`.interview_reports (session_id VARCHAR(128) CHARACTER SET ascii COLLATE ascii_bin PRIMARY KEY,candidate_name VARCHAR(120) NOT NULL,report_json JSON NOT NULL,created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;")
            sql(f"INSERT INTO `{name}`.interview_reports(session_id,candidate_name,report_json) VALUES('legacy','legacy',JSON_OBJECT('summary','preserved'));")
            backend = processes.start(backend_args)
            gateway = processes.start(gateway_args)
            assert sql(f"SELECT JSON_UNQUOTE(JSON_EXTRACT(report_json,'$.summary')) FROM `{name}`.interview_reports WHERE session_id='legacy';").strip()=='preserved'
            client = Client(http_port)
            assert client.request('/', method='GET')[0] == 200
            assert client.request('/src/app.js', method='GET')[0] == 200
            assert client.request('/%2e%2e/server/config/default_config.json', method='GET')[0] == 404
            assert client.request('/api/account/login', {}, origin='https://wrong.example')[0] == 403
            try:
                WebSocket(client, origin='https://wrong.example')
                raise AssertionError('cross-origin websocket was accepted')
            except PermissionError:
                pass
            credentials = {'email': 'first@example.test', 'password': 'StrongTestPassword123!', 'display_name': 'First'}
            first, headers = client.post('/api/account/register', credentials)
            assert 'access_token' not in first and 'HttpOnly' in headers['Set-Cookie']
            cookie = headers['Set-Cookie'].split(';')[0]
            client.post('/api/account/register', credentials, expected=409)
            client.post('/api/account/login', {**credentials, 'password': 'wrong-password'}, expected=401)
            me, _ = client.post('/api/account/me', {}, cookie)
            assert me['user']['id'] == first['user']['id']
            second, h = client.post('/api/account/register', {**credentials, 'email': 'second@example.test', 'display_name': 'Second'})
            second_cookie = h['Set-Cookie'].split(';')[0]
            # 从 Node.js scrypt 格式迁移的固定盐哈希，验证旧用户无需重注册。
            salt = '00112233445566778899aabbccddeeff'
            key = hashlib.scrypt(credentials['password'].encode(), salt=salt.encode(), n=32768, r=8, p=1, maxmem=64*1024*1024, dklen=64).hex()
            sql(f"UPDATE `{name}`.users SET password_hash='scrypt${salt}${key}' WHERE id='{first['user']['id']}';")
            _, login_headers = client.post('/api/account/login', credentials)
            cookie = login_headers['Set-Cookie'].split(';')[0]
            ws = WebSocket(client, cookie)
            assert ws.receive()['type'] == 'connected'
            ws.send(b'{bad json')
            assert ws.receive()['type'] == 'error'
            ws.send({'type': 'create_session', 'candidate_name': 'WebSocket Candidate', 'question_total': 1})
            created = ws.receive()
            assert created['type'] == 'session'
            assert ws.receive()['event_type'] == 'QUESTION'
            assert ws.receive()['type'] == 'done'
            ws.close()
            resume = base64.b64encode((ROOT/'resource/resume.pdf').read_bytes()).decode()
            client.post('/api/sessions', {'candidate_name': 'bad', 'question_total': 1, 'resume_pdf': 'invalid'}, cookie, expected=400)
            client.post('/api/sessions', {'candidate_name': 'anon', 'question_total': 1}, expected=401)
            events = client.events('/api/sessions', {'candidate_name': 'PDF Candidate', 'question_total': 1, 'resume_pdf': resume}, cookie)
            drafts = [event for event in events if event.get('event_type') == 'DELTA']
            assert len(drafts) > 1
            session = next(event for event in events if event['type'] == 'session')
            question = next(event for event in events if event.get('event_type') == 'QUESTION')
            assert drafts[-1]['text'] == question['text'] and events[-1]['type'] == 'done'
            failed = client.events('/api/answer', {'session_id': session['id'], 'text': 'wrong owner'}, second_cookie)
            assert failed[-1]['type'] == 'error'
            answer = client.events('/api/answer', {'session_id': session['id'], 'text': 'RAII 通过对象生命周期释放资源。'}, cookie)
            assert any(e.get('event_type') == 'SUMMARY' for e in answer) and answer[-1]['type'] == 'done'
            history, _ = client.post('/api/reports', {}, cookie)
            assert any(row['session_id'] == session['id'] for row in history['reports'])
            assert client.post('/api/reports', {}, second_cookie)[0]['reports'] == []
            report = client.events('/api/report', {'session_id': session['id']}, cookie)
            assert next(e['report'] for e in report if e['type'] == 'report')['owner_user_id'] == first['user']['id']
            forbidden = client.events('/api/report', {'session_id': session['id']}, second_cookie)
            assert forbidden[-1]['type'] == 'error' and not any(e['type'] == 'report' for e in forbidden)
            if args.qt:
                env = {**os.environ, 'QT_QPA_PLATFORM': 'offscreen', 'TEST_ACCOUNT_API': client.origin,
                       'TEST_GRPC_TARGET': f'127.0.0.1:{grpc_port}', 'TEST_ACCOUNT_EMAIL': credentials['email'], 'TEST_ACCOUNT_PASSWORD': credentials['password']}
                subprocess.run([str(args.qt)], env=env, check=True, timeout=30)
            processes.stop(gateway)
            processes.stop(backend)
            backend = processes.start(backend_args)
            gateway = processes.start(gateway_args)
            assert client.post('/api/account/me', {}, cookie)[0]['user']['id'] == first['user']['id']
            assert client.events('/api/report', {'session_id': session['id']}, cookie)[-1]['type'] == 'done'
            client.post('/api/account/logout', {}, cookie)
            client.post('/api/account/me', {}, cookie, expected=401)
            # idle websocket must not prevent a clean SIGTERM.
            idle = WebSocket(client, second_cookie)
            assert idle.receive()['type'] == 'connected'
            processes.stop(gateway)
            idle.close()
            print('Protocol-only gateway (no database config): HTTP/WebSocket/SSE, backend accounts, PDF, user isolation, persistence, restart/logout and Qt compatibility passed')
    finally:
        processes.close()
        sql(f'DROP DATABASE IF EXISTS `{name}`;')
        # 只清理本脚本创建的独立测试命名空间。
        keys = subprocess.run(['redis-cli', '--scan', '--pattern', prefix+'*'], text=True, capture_output=True, check=True).stdout.splitlines()
        if keys:
            subprocess.run(['redis-cli', 'DEL', *keys], check=True, capture_output=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--backend', type=Path, required=True)
    parser.add_argument('--gateway', type=Path, required=True)
    parser.add_argument('--qt', type=Path)
    run(parser.parse_args())
