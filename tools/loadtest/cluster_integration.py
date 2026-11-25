#!/usr/bin/env python3
"""Real ZooKeeper + two backend processes, isolated MySQL/Redis, no gateway/model calls."""
import argparse, collections, concurrent.futures, http.client, json, os, secrets, socket, subprocess, tempfile, time
from pathlib import Path
from grpc_web import Client
from run_upgrade import port, sql, ROOT

def wait(test, seconds=20):
    until=time.monotonic()+seconds
    while time.monotonic()<until:
        try:
            value=test()
            if value:return value
        except (OSError,RuntimeError):pass
        time.sleep(.2)
    raise AssertionError('timed out waiting for expected cluster state')

def members(port):
    connection=http.client.HTTPConnection('127.0.0.1',port,timeout=2)
    try:
        connection.request('GET','/healthz')
        return json.loads(connection.getresponse().read())['members']
    finally:connection.close()

def start_session(client):
    events=client.call('StartInterview',{1:'Synthetic cluster test',3:1})
    assert any(e[1]==7 for e in events)
    return next(e[2].decode() for e in events if e[1]==8)

def finish(client,sid):
    events=client.call('SendText',{1:sid,2:'RAII ties resources to object lifetime.'})
    assert any(e[1]==5 for e in events)
    assert json.loads(client.call('GetReport',{1:sid})[0][2])['session_id']==sid

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--backend',required=True);parser.add_argument('--grpc-bench',required=True);parser.add_argument('--output');args=parser.parse_args()
    name='ai_cluster_'+secrets.token_hex(5);prefix=name+':';root='/'+name
    children=[];logs=[];gp=[port(),port()];wp=[port(),port()]
    try:
        with tempfile.TemporaryDirectory() as directory:
            def launch(i):
                config={'storage':{'enabled':True,'mysql':{'host':'127.0.0.1','user':'root','database':name},'redis':{'prefix':prefix}},'cluster':{'instance_id':f'node-{i}','advertised_address':f'127.0.0.1:{gp[i]}','zookeeper':{'enabled':True,'hosts':'127.0.0.1:2181','path':root}},'rabbitmq':{'enabled':True}}
                path=Path(directory)/f'{i}.json';path.write_text(json.dumps(config));path.chmod(0o600);log=open(Path(directory)/f'{i}.log','a+');logs.append(log)
                process=subprocess.Popen([str(Path(args.backend).resolve()),'--mock-model','--config',str(path),'--listen',f'127.0.0.1:{gp[i]}','--web-listen',f'127.0.0.1:{wp[i]}'],cwd=ROOT,stdout=log,stderr=log);children.append(process)
                def ready():
                    c=http.client.HTTPConnection('127.0.0.1',wp[i],timeout=1)
                    try:c.request('GET','/healthz');return c.getresponse().status==200
                    finally:c.close()
                wait(ready);return process
            first=launch(0);second=launch(1)
            client=Client(wp[0]);credentials={'email':name+'@example.test','password':'Synthetic-test-123','display_name':'Cluster'};client.account('register',credentials)
            other=Client(wp[1],client.cookie);assert other.account('me')['user']['email']==credentials['email']
            # Wait for both views to include both nodes, then round robin through one entry.
            def both():
                ids=[start_session(client) for _ in range(4)]
                return ids if len({sid.split('_')[0] for sid in ids})==2 else None
            wait(lambda:members(wp[0])==['node-0','node-1'] and members(wp[1])==['node-0','node-1'])
            wait(both)
            sessions=[start_session(client) for _ in range(12)]
            counts=dict(collections.Counter(sid.split('_')[0] for sid in sessions));assert counts=={'node-0':6,'node-1':6},counts
            # Answers reach the owner even when sent through the opposite entry.
            with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
                list(pool.map(lambda sid:finish(other if sid.startswith('node-0_') else client,sid),sessions))
            assert len(json.loads(other.call('ListReports',{1:0})[0][1]))>=12
            # Native gRPC client also enters through node-0 and uses the same routing policy.
            connection=http.client.HTTPConnection('127.0.0.1',wp[0]);connection.request('POST','/api/account/native-login',json.dumps(credentials),{'Content-Type':'application/json'});token=json.loads(connection.getresponse().read())['access_token'];connection.close()
            env=os.environ.copy();env['BENCH_AUTH_TOKEN']=token
            native=json.loads(subprocess.check_output([str(Path(args.grpc_bench).resolve()),f'127.0.0.1:{gp[0]}','4','12'],env=env,text=True));assert native['cycle']['success']==12
            native_counts=dict(line.split('\t') for line in sql(f"SELECT SUBSTRING_INDEX(session_id,'_',1),COUNT(*) FROM {name}.interview_reports WHERE candidate_name='Load test synthetic' GROUP BY 1").splitlines());assert native_counts=={'node-0':'6','node-1':'6'},native_counts
            pending=wait(lambda:next((sid for sid in [start_session(client),start_session(client)] if sid.startswith('node-1_')),None))
            second.kill();second.wait(timeout=5)
            # No replay or migration for a lost in-memory interview.
            try:finish(client,pending);raise AssertionError('lost owner accepted answer')
            except RuntimeError as error:assert 'gRPC 14' in str(error),str(error)
            def removed():
                ids=[start_session(client) for _ in range(4)]
                return all(sid.startswith('node-0_') for sid in ids)
            wait(lambda:members(wp[0])==['node-0'],25)
            wait(removed,25)
            second=launch(1)
            wait(lambda:members(wp[0])==['node-0','node-1'] and members(wp[1])==['node-0','node-1'],25)
            wait(both,25)
            restored=[start_session(client) for _ in range(8)];assert len({s.split('_')[0] for s in restored})==2
            # A restarted process cannot restore an old in-memory session.
            try:finish(client,pending);raise AssertionError('old session restored unexpectedly')
            except RuntimeError as error:assert 'gRPC 5' in str(error)
            assert json.loads(other.call('GetReport',{1:sessions[0]})[0][2])['session_id']==sessions[0]
            result={'round_robin':counts,'cross_entry_completed':12,'native_completed':12,'native_distribution':native_counts,'owner_failure_rejected':True,'node_restart_rejoined':True,'persisted_report_after_restart':True}
            if args.output:Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
            print(json.dumps(result))
    finally:
        for child in children:
            if child.poll() is None:child.terminate()
        for child in children:
            try:child.wait(timeout=10)
            except subprocess.TimeoutExpired:child.kill();child.wait()
        for log in logs:log.close()
        sql('DROP DATABASE IF EXISTS '+name)
        keys=subprocess.check_output(['redis-cli','--scan','--pattern',prefix+'*'],text=True).splitlines()
        if keys:subprocess.run(['redis-cli','DEL',*keys],capture_output=True,check=True)
        # Delete only our isolated ZooKeeper persistent parent after ephemeral nodes expire.
        cli=Path('/opt/homebrew/bin/zkCli');
        if cli.exists():
            subprocess.run([str(cli),'-server','127.0.0.1:2181','deleteall',root],capture_output=True,timeout=20)
if __name__=='__main__':main()
