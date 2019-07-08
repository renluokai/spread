import tkinter as tk
import tkinter.ttk as ttk

class Runtime():
	def __init__(self,master):
		label_stick=tk.W
		root = tk.LabelFrame(master,text='runtime report')
		label=ttk.Label(root,text='价差计算方式')
		label.grid(row=0,column=0)
		spread_type=ttk.Label(root,text='远期-近期')
		spread_type.grid(row=1,column=0)
		
		label=ttk.Label(root,text='买价价差')
		label.grid(row=0,column=3)
		bid_spread=ttk.Label(root,text='等待更新')
		bid_spread.grid(row=0,column=4)
		
		label=ttk.Label(root,text='卖价价差')
		label.grid(row=1,column=3)
		ask_spread=ttk.Label(root,text='等待更新')
		ask_spread.grid(row=1,column=4)
		
		label=ttk.Label(root,text='运行状态')
		label.grid(row=0,column=6)
		run_status=ttk.Label(root,text='等待指令')
		run_status.grid(row=1,column=6)
		
		separator=tk.Frame(root,height=15)
		separator.grid(row=2,column=0,columnspan=8,stick=tk.W+tk.E+tk.N+tk.S)
		
		label=ttk.Label(root, text='当前持仓价差')
		label.grid(row=3)
		current=tk.Listbox(root)
		current.grid(row=4,column=0,columnspan=2,stick=tk.W+tk.E)
		current.insert(tk.END,'价差价格/手数/开仓时间')
		label=ttk.Label(root, text='手数合计')
		label.grid(row=5)
		current_summary=tk.Entry(root)
		current_summary.grid(row=5,column=1,stick=tk.W+tk.E)
		separator=tk.Frame(root,width=25)
		separator.grid(row=3,rowspan=3,column=2,stick=tk.W+tk.E+tk.N+tk.S)
		
		label=ttk.Label(root, text='止盈记录')
		label.grid(row=3,column=3)
		current=tk.Listbox(root)
		current.grid(row=4,column=3,columnspan=2,stick=tk.W+tk.E)
		current.insert(tk.END,'止盈价差/手数/平仓时间')
		label=ttk.Label(root, text='手数合计')
		label.grid(row=5,column=3)
		current_summary=tk.Entry(root)
		current_summary.grid(row=5,column=4)
		separator=tk.Frame(root,width=25)
		separator.grid(row=3,rowspan=3,column=5,stick=tk.W+tk.E+tk.N+tk.S)
		
		label=ttk.Label(root, text='止损记录')
		label.grid(row=3,column=6)
		current=tk.Listbox(root)
		current.grid(row=4,column=6,columnspan=2,stick=tk.W+tk.E)
		current.insert(tk.END,'止损价差/手数/平仓时间')
		label=ttk.Label(root, text='手数合计')
		label.grid(row=5,column=6)
		current_summary=tk.Entry(root)
		current_summary.grid(row=5,column=7)
		root.pack()
if __name__=='__main__':
	root =tk.Tk()
	my_runtime=Runtime(root)
	tk.mainloop()
