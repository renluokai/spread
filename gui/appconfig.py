import tkinter as tk
import tkinter.ttk as ttk

class Config():
	def __init__(self, root):
		r=0
		label_stick=tk.W
		root = tk.Frame(root)

		label=ttk.Label(root, text='用户代码')
		user=ttk.Entry(root)
		label.grid(row=r,column=0,stick=label_stick)
		user.grid(row=r,column=1,stick=tk.W+tk.E)
		r+=1

		label=ttk.Label(root, text='交易密码')
		password=ttk.Entry(root)
		label.grid(row=r,column=0,stick=label_stick)
		password.grid(row=r,column=1,stick=tk.W+tk.E)
		r+=1

		label=ttk.Label(root, text='经纪商')
		brokerid=ttk.Entry(root)
		label.grid(row=r,column=0,stick=label_stick)
		brokerid.grid(row=r,column=1,stick=tk.W+tk.E)
		r+=1

		label=ttk.Label(root, text='近月合约')
		recent_contact=ttk.Combobox(root,values=['任洛凯','张凤莉','任予心'])
		label.grid(row=r,column=0,stick=label_stick)
		recent_contact.grid(row=r,column=1)
		r+=1

		label=ttk.Label(root, text='远月合约')
		forward_contract=ttk.Combobox(root,values=['任洛凯','张凤莉','任予心'])
		label.grid(row=r,column=0,stick=label_stick)
		forward_contract.grid(row=r,column=1)
		r+=1

		label=ttk.Label(root, text='开仓一腿')
		pen_from=ttk.Combobox(root,values=['远期','近期'])
		label.grid(row=r,column=0,stick=label_stick)
		pen_from.grid(row=r,column=1)
		r+=1
		
		label=ttk.Label(root, text='平仓一腿')
		close_from=ttk.Combobox(root,values=['远期','近期'])
		label.grid(row=r,column=0,stick=label_stick)
		close_from.grid(row=r,column=1)
		r+=1
		
		label=ttk.Label(root, text='价差触发')
		trigger_spread=ttk.Combobox(root,values=['远期','近期'])
		label.grid(row=r,column=0,stick=label_stick)
		trigger_spread.grid(row=r,column=1)
		r+=1
		
		label=ttk.Label(root, text='买卖数量比率')
		volume_ratio=ttk.Combobox(root,values=[str(x)+'%' for x in range(0,100)])
		label.grid(row=r,column=0,stick=label_stick)
		volume_ratio.grid(row=r,column=1)
		r+=1
		
		label=ttk.Label(root, text='价差涨跌')
		direction=ttk.Combobox(root,values=['涨','跌'])
		label.grid(row=r,column=0,stick=label_stick)
		direction.grid(row=r,column=1)
		r+=1
		
		
		label=ttk.Label(root, text='开仓价差')
		open_threshold=ttk.Entry(root)
		label.grid(row=r,column=0,stick=label_stick)
		open_threshold.grid(row=r,column=1,stick=tk.W+tk.E)
		r+=1
		
		label=ttk.Label(root, text='平仓价差')
		close_threshold=ttk.Entry(root)
		label.grid(row=r,column=0,stick=label_stick)
		close_threshold.grid(row=r,column=1,stick=tk.W+tk.E)
		r+=1
		
		label=ttk.Label(root, text='止损跳数')
		stop_loss=ttk.Combobox(root,values=[x for x in range(0,100)])
		label.grid(row=r,column=0,stick=label_stick)
		stop_loss.grid(row=r,column=1)
		r+=1
		
		label=ttk.Label(root, text='止损类型')
		stop_loss_type=ttk.Combobox(root,values=['均价止损','逐笔止损','不设止损'])
		label.grid(row=r,column=0,stick=label_stick)
		stop_loss_type.grid(row=r,column=1)
		r+=1
		
		label=ttk.Label(root, text='开仓手数')
		max_open=ttk.Combobox(root,values=[x for x in range(0,1000)])
		label.grid(row=r,column=0,stick=label_stick)
		max_open.grid(row=r,column=1)
		r+=1
		
		label=ttk.Label(root, text='单次报单数量')
		submit_max=ttk.Combobox(root,values=[x for x in range(0,1000)])
		label.grid(row=r,column=0,stick=label_stick)
		submit_max.grid(row=r,column=1)
		r+=1
		
		
		label=ttk.Label(root, text='交易前置')
		trade_front=ttk.Entry(root)
		label.grid(row=r,column=0,stick=label_stick)
		trade_front.grid(row=r,column=1,stick=tk.W+tk.E)
		r+=1
		
		label=ttk.Label(root, text='行情前置')
		quote_front=ttk.Entry(root)
		label.grid(row=r,column=0,stick=label_stick)
		quote_front.grid(row=r,column=1,stick=tk.W+tk.E)
		r+=1
		
		separator=tk.Frame(root,width=25)
		separator.grid(row=0,rowspan=r,column=2,stick=tk.W+tk.E+tk.N+tk.S)
		
		one_key_stop=tk.Button(root, text='停止',activebackground='red',bg='red')
		one_key_stop.grid(row=0,column=3,rowspan=4,columnspan=4,stick=tk.W+tk.E+tk.N+tk.S)
		
		start=tk.Button(root, text='开始',activebackground='green',bg='green')
		start.grid(row=4,column=3,rowspan=4,columnspan=4,stick=tk.W+tk.E+tk.N+tk.S)
		
		separator=tk.Frame(root,width=25)
		separator.grid(row=0,rowspan=r,column=8,stick=tk.W+tk.E+tk.N+tk.S)
		
		root.pack()

if __name__=='__main__':
	root =tk.Tk()
	my_config=Config(root)
	tk.mainloop()
