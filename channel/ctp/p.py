#! /usr/bin/python
import sys
def parse(f, prompt=None):
	lines = f.readlines()
	if prompt == None:
		prompt = "xxx"
	for line in lines:
		l = line.strip('	; ')
		if l.startswith('///'):
			continue;
		print('cout<<"{}="<<{}->{}<<endl;'.format(l.split()[-1][:-1],prompt, l.split()[-1][:-1]))
		#print('cout<<"{}="<<{}->{}'.format(l.split()[-1][:-1],prompt, l.split()[-1]))
	return True

if __name__ == '__main__':
	file_name = 'target.text'
	f = open(file_name, 'r')
	prompt = None

	if(len(sys.argv) == 2):
		prompt = sys.argv[1]
	ret = parse(f, prompt)
	
