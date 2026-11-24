#!/usr/bin/env python3
"""Isolated real MySQL/Redis, mock model, native gRPC + gRPC-Web benchmark."""
import argparse, concurrent.futures, json, os, secrets, socket, subprocess, tempfile, time
from pathlib import Path
from grpc_web import Client
ROOT=Path(__file__).resolve().parents[2]
def port():
    with socket.socket() as s:
        s.bind(('127.0.0.1',0));return s.getsockname()[1]
def sql(text):
    return subprocess.run(['mysql','-h','127.0.0.1','-u','root','-N'],input=text,text=True,check=True,capture_output=True).stdout

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--backend',required=True);parser.add_argument('--grpc-bench',required=True);parser.add_argument('--output',required=True);args=parser.parse_args()
    schema='ai_upgrade_'+secrets.token_hex(5);prefix=schema+':'
    config={'storage':{'enabled':True,'mysql':{'host':'127.0.0.1','user':'root','database':schema,'pool_size':8},'redis':{'prefix':prefix,'pool_size':16}},'llm':{},'rabbitmq':{'enabled':True}}
    results=[];process=None
    try:
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'config.json';path.write_text(json.dumps(config));path.chmod(0o600)
            for concurrency in [1,8,32,64,128,160]:
                config['storage']['redis']['prefix']=prefix+str(concurrency)+':'
                path.write_text(json.dumps(config))
                gp,wp=port(),port();log=open(Path(temp)/'server.log','w+')
                process=subprocess.Popen([str(Path(args.backend).resolve()),'--config',str(path),'--mock-model','--listen',f'127.0.0.1:{gp}','--web-listen',f'127.0.0.1:{wp}'],cwd=ROOT,stdout=log,stderr=log)
                until=time.monotonic()+20
                while True:
                    try:
                        client=Client(wp);client.account('me');break
                    except RuntimeError as e:
                        if 'gRPC 16' in str(e):break
                    except OSError:pass
                    if process.poll() is not None or time.monotonic()>until:
                        raise RuntimeError('backend failed to start')
                    time.sleep(.1)
                user=client.account('register',{'email':secrets.token_hex(5)+'@example.test','password':'Synthetic-test-123','display_name':'Synthetic'})
                assert 'token' not in user,'web token leaked'
                # Qt-compatible token issued by native HTTP login, never written to output.
                import http.client
                c=http.client.HTTPConnection('127.0.0.1',wp)
                # Use a separate registration for native gRPC bearer testing.
                c.request('POST','/api/account/native-register',json.dumps({'email':secrets.token_hex(5)+'@example.test','password':'Synthetic-test-123','display_name':'Native'}),{'Content-Type':'application/json'})
                token=json.loads(c.getresponse().read())['access_token'];c.close()
                env=os.environ.copy();env['BENCH_AUTH_TOKEN']=token
                count=max(100,concurrency*2)
                native=json.loads(subprocess.check_output([str(Path(args.grpc_bench).resolve()),f'127.0.0.1:{gp}',str(concurrency),str(count)],env=env,text=True))
                latencies=[];errors={};start=time.monotonic()
                def run(_):
                    begin=time.monotonic()
                    try:Client(wp,client.cookie).cycle();return (time.monotonic()-begin)*1000,None
                    except Exception as e:return (time.monotonic()-begin)*1000,str(e)
                with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as pool:
                    for ms,error in pool.map(run,range(count)):
                        latencies.append(ms)
                        if error:errors[error]=errors.get(error,0)+1
                elapsed=time.monotonic()-start;latencies.sort();ok=count-sum(errors.values())
                item={'concurrency':concurrency,'count':count,'native':native,'web':{'success':ok,'errors':errors,'successful_cycles_per_second':ok/elapsed,'p95_ms':latencies[int(.95*len(latencies))]}}
                # Ownership and persisted-history checks.
                sid=client.cycle();assert json.loads(client.call('ListReports',{1:0})[0][1])
                if concurrency==1:
                    other=Client(wp);other.account('register',{'email':secrets.token_hex(5)+'@example.test','password':'Synthetic-test-123','display_name':'Other'})
                    try:other.call('GetReport',{1:sid});raise AssertionError('cross-user report allowed')
                    except RuntimeError as e:assert 'gRPC 5' in str(e)
                client.account('logout')
                try:client.call('ListReports',{1:0});raise AssertionError('logout did not revoke cookie')
                except RuntimeError as e:assert 'gRPC 16' in str(e)
                if concurrency==1:
                    node_env=os.environ.copy();node_env['INTERVIEW_TEST_URL']=f'http://127.0.0.1:{wp}'
                    subprocess.run(['node','web/tests/live-rpc.mjs'],cwd=ROOT,env=node_env,check=True)
                results.append(item);print(json.dumps(item),flush=True)
                process.terminate();process.wait(timeout=10);process=None;log.close()
            counts=sql(f'SELECT (SELECT COUNT(*) FROM {schema}.interview_reports),(SELECT COUNT(*) FROM {schema}.report_outbox)').strip().split()
            assert counts[0]==counts[1], 'report/outbox counts differ'
            Path(args.output).parent.mkdir(parents=True,exist_ok=True);Path(args.output).write_text(json.dumps(results,indent=2)+'\n')
    finally:
        if process is not None:process.terminate();process.wait(timeout=10)
        sql('DROP DATABASE IF EXISTS '+schema)
        keys=subprocess.check_output(['redis-cli','--scan','--pattern',prefix+'*'],text=True).splitlines()
        if keys:subprocess.run(['redis-cli','DEL',*keys],check=True,capture_output=True)
if __name__=='__main__':main()
