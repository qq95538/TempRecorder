using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace SerialCom
{

    struct LogLine
    {
        public int index;
        public DateTime time;
        public float temperature;
        public float moisture;
    }
    public partial class MainForm : Form
    {
        public static AutoResetEvent autoReadDataConfirmedEvent = new AutoResetEvent(false);
        public delegate void InvokeSerialSendText(String sText);
        public delegate void InvokeRefreshData(int index, DateTime time, float temperature, float moisture);
        public delegate void InvokeLockReadDataButton(bool bLockButton);
        public delegate void InvokeAdjustChart();
        //实例化串口对象
        SerialPort serialPort = new SerialPort();

        String saveDataFile = null;
        FileStream saveDataFS = null;
        List<String> listString;
        List<LogLine> listLogLine;


        public MainForm()
        {
            InitializeComponent();
            listString = new List<String>();
            listLogLine = new List<LogLine>();

        }


        //初始化串口界面参数设置
        private void Init_Port_Confs()
        {
            /*------串口界面参数设置------*/

            //检查是否含有串口
            string[] str = SerialPort.GetPortNames();
            if (str == null)
            {
                MessageBox.Show("本机没有串口！", "Error");
                return;
            }
            //添加串口
            foreach (string s in str)
            {
                comboBoxCom.Items.Add(s);
            }
            if(comboBoxCom.Items.Count > 0)
            {
                comboBoxCom.SelectedIndex = comboBoxCom.Items.Count - 1;
            }


            /*------波特率设置-------*/
            string[] baudRate = { "9600", "19200", "38400", "57600", "115200" };
            foreach (string s in baudRate)
            {
                comboBoxBaudRate.Items.Add(s);
            }
            comboBoxBaudRate.SelectedIndex = 1;

            /*------数据位设置-------*/
            string[] dataBit = { "5", "6", "7", "8" };
            foreach (string s in dataBit)
            {
                comboBoxDataBit.Items.Add(s);
            }
            comboBoxDataBit.SelectedIndex = 3;


            /*------校验位设置-------*/
            string[] checkBit = { "None", "Even", "Odd", "Mask", "Space" };
            foreach (string s in checkBit)
            {
                comboBoxCheckBit.Items.Add(s);
            }
            comboBoxCheckBit.SelectedIndex = 0;


            /*------停止位设置-------*/
            string[] stopBit = { "1", "1.5", "2" };
            foreach (string s in stopBit)
            {
                comboBoxStopBit.Items.Add(s);
            }
            comboBoxStopBit.SelectedIndex = 0;

        }

        //加载主窗体
        private void MainForm_Load(object sender, EventArgs e)
        {

            Init_Port_Confs();

            Control.CheckForIllegalCrossThreadCalls = false;
            serialPort.DataReceived += new SerialDataReceivedEventHandler(dataReceived);


            //准备就绪              
            serialPort.DtrEnable = true;
            serialPort.RtsEnable = true;
            //设置数据读取超时为1秒
            serialPort.ReadTimeout = 1000;
            serialPort.Close();
            buttonSendData.Enabled = false;

        }


        //打开串口 关闭串口
        private void buttonOpenCloseCom_Click(object sender, EventArgs e)
        {
            OpenSerial();
            buttonCleanSD.Enabled = true;
        }

        //接收数据
        private void dataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            try
            {
                String input = "";
                do {
                    input = serialPort.ReadLine();
                    listString.Add(input);
                    textBoxReceive.Text += input + "\r\n";
                } while (serialPort.BytesToRead > 0);
                if (listString.Count > 0)
                {
                    autoReadDataConfirmedEvent.Set();
                }
                

                // save data to file
                textBoxReceive.SelectionStart = textBoxReceive.Text.Length;
                textBoxReceive.ScrollToCaret();//滚动到光标处
                if (saveDataFS != null)
                {
                    byte[] info = new UTF8Encoding(true).GetBytes(input + "\r\n");
                    saveDataFS.Write(info, 0, info.Length);
                }
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message, "Serial error");
                return;
            }
        }

        //发送数据
        private void buttonSendData_Click(object sender, EventArgs e)
        {
            try
            {
                serialPort.WriteLine(textBoxSend.Text.Trim());
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message, "Serial error when sending a command");
                return;
            }
        }

        //清空接收数据框
        private void buttonClearRecData_Click(object sender, EventArgs e)
        {
            textBoxReceive.Text = "";
        }


        //窗体关闭时
        private void MainForm_Closing(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                serialPort.Close();//关闭串口
            }

            if (saveDataFS != null)
            {
                saveDataFS.Close(); // 关闭文件
                saveDataFS = null;//释放文件句柄
            }

        }

        //刷新串口
        private void Button_Refresh_Click(object sender, EventArgs e)
        {
            comboBoxCom.Text = "";
            comboBoxCom.Items.Clear();

            string[] str = SerialPort.GetPortNames();
            if (str == null)
            {
                MessageBox.Show("本机没有串口！", "Error");
                return;
            }

            //添加串口
            foreach (string s in str)
            {
                comboBoxCom.Items.Add(s);
            }

            //设置默认串口
            if(comboBoxCom.Items.Count > 0) {
                comboBoxCom.SelectedIndex = comboBoxCom.Items.Count - 1;
            }
            comboBoxBaudRate.SelectedIndex = 1;
            comboBoxDataBit.SelectedIndex = 3;
            comboBoxCheckBit.SelectedIndex = 0;
            comboBoxStopBit.SelectedIndex = 0;

        }

        // 退出
        private void ExitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (serialPort.IsOpen)
            {
                serialPort.Close();//关闭串口
            }
            if (saveDataFS != null)
            {
                saveDataFS.Close(); // 关闭文件
                saveDataFS = null;//释放文件句柄
            }

            this.Close();
        }

        // 重置串口参数设置
        private void ResetPortConfToolStripMenuItem_Click(object sender, EventArgs e)
        {
            comboBoxCom.SelectedIndex = 0;
            comboBoxBaudRate.SelectedIndex = 0;
            comboBoxDataBit.SelectedIndex = 3;
            comboBoxCheckBit.SelectedIndex = 0;
            comboBoxStopBit.SelectedIndex = 0;
        }

        // 保存接收数据到文件
        private void SaveReceiveDataToFileToolStripMenuItem_Click(object sender, EventArgs e)
        {
            SaveFileDialog saveFileDialog = new SaveFileDialog();
            saveFileDialog.Filter = "Txt |*.txt";
            saveFileDialog.Title = "保存接收到的数据到文件中";
            saveFileDialog.ShowDialog();

            if (saveFileDialog.FileName != null)
            {
                saveDataFile = saveFileDialog.FileName;
            }


        }

        private void textBoxSend_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                try
                {
                    serialPort.WriteLine(textBoxSend.Text.Trim());
                    textBoxSend.Text = "";
                }
                catch (System.Exception ex)
                {
                    MessageBox.Show(ex.Message, "Serial error when sending a command");
                    return;
                }
            }
        }



        private void setClockAndCleanSD()
        {
            System.DateTime currentTime = new System.DateTime();
            currentTime = System.DateTime.Now;
            InvokeSerialSendText invokeSerialSendText = new InvokeSerialSendText(SendTextToSerial);
            this.BeginInvoke(invokeSerialSendText, new Object[] { "CLOCK " + currentTime.Year
                    + " " + currentTime.Month
                    + " " + currentTime.Day
                    + " " + currentTime.Hour
                    + " " + currentTime.Minute
                    + " " + currentTime.Second });
            if (autoReadDataConfirmedEvent.WaitOne())
            {
                listString.Clear();
                this.BeginInvoke(invokeSerialSendText, new Object[] { "DEL" });
                if (autoReadDataConfirmedEvent.WaitOne())
                {
                    this.BeginInvoke(invokeSerialSendText, new Object[] { "NEW" });
                }
            }

        }

        private void SendTextToSerial(String iText)
        {
            try
            {
                serialPort.WriteLine(iText);
            }
            catch (System.Exception ex)
            {
                MessageBox.Show(ex.Message, "Serial error when sending a command");
                return;
            }
        }

        private void OpenSerial()
        {
            if (!serialPort.IsOpen)//串口处于关闭状态
            {
                try
                {
                    if (comboBoxCom.SelectedIndex == -1)
                    {
                        MessageBox.Show("Error: 无效的端口,请重新选择", "Error");
                        return;
                    }
                    string strSerialName = comboBoxCom.SelectedItem.ToString();
                    string strBaudRate = comboBoxBaudRate.SelectedItem.ToString();
                    string strDataBit = comboBoxDataBit.SelectedItem.ToString();
                    string strCheckBit = comboBoxCheckBit.SelectedItem.ToString();
                    string strStopBit = comboBoxStopBit.SelectedItem.ToString();
                    Int32 iBaudRate = Convert.ToInt32(strBaudRate);
                    Int32 iDataBit = Convert.ToInt32(strDataBit);
                    serialPort.PortName = strSerialName;//串口号
                    serialPort.BaudRate = iBaudRate;//波特率
                    serialPort.DataBits = iDataBit;//数据位
                    switch (strStopBit)            //停止位
                    {
                        case "1":
                            serialPort.StopBits = StopBits.One;
                            break;
                        case "1.5":
                            serialPort.StopBits = StopBits.OnePointFive;
                            break;
                        case "2":
                            serialPort.StopBits = StopBits.Two;
                            break;
                        default:
                            MessageBox.Show("Error：停止位参数不正确!", "Error");
                            break;
                    }
                    switch (strCheckBit)             //校验位
                    {
                        case "None":
                            serialPort.Parity = Parity.None;
                            break;
                        case "Odd":
                            serialPort.Parity = Parity.Odd;
                            break;
                        case "Even":
                            serialPort.Parity = Parity.Even;
                            break;
                        default:
                            MessageBox.Show("Error：校验位参数不正确!", "Error");
                            break;
                    }
                    if (saveDataFile != null)
                    {
                        saveDataFS = File.Create(saveDataFile);
                    }
                    //打开串口
                    serialPort.Open();
                    //打开串口后设置将不再有效
                    comboBoxCom.Enabled = false;
                    comboBoxBaudRate.Enabled = false;
                    comboBoxDataBit.Enabled = false;
                    comboBoxCheckBit.Enabled = false;
                    comboBoxStopBit.Enabled = false;
                    buttonSendData.Enabled = true;
                    Button_Refresh.Enabled = false;
                    buttonOpenCloseCom.Text = "记录仪已连接";
                }
                catch (System.Exception ex)
                {
                    MessageBox.Show("Error:" + ex.Message, "Error");
                    return;
                }
            }
        }

        private void CloseSerial()
        {
            if (serialPort.IsOpen)//串口处于关闭状态
            {

                serialPort.Close();//关闭串口
                //串口关闭时设置有效
                comboBoxCom.Enabled = true;
                comboBoxBaudRate.Enabled = true;
                comboBoxDataBit.Enabled = true;
                comboBoxCheckBit.Enabled = true;
                comboBoxStopBit.Enabled = true;
                buttonSendData.Enabled = false;
                Button_Refresh.Enabled = true;
                buttonCleanSD.Enabled = false;
                buttonOpenCloseCom.Text = "连接记录仪";
                if (saveDataFS != null)
                {
                    saveDataFS.Close(); // 关闭文件
                    saveDataFS = null;//释放文件句柄
                }
            }    
        }

        private void CloseSerialButton_Click(object sender, EventArgs e)
        {
            CloseSerial();
        }

        private void ReadDataButton_Click(object sender, EventArgs e)
        {
            listString.Clear();
            listLogLine.Clear();
            
            Thread threadReadData = new Thread(new ThreadStart(readData));
            threadReadData.Start();
            
        }

        private void readData()
        {
            int read_pointer = 0;
            int count = 0;

            InvokeLockReadDataButton invokeLockReadDataButton = new InvokeLockReadDataButton(lockReadDataButton);
            this.BeginInvoke(invokeLockReadDataButton, new Object[] { true });

            InvokeSerialSendText invokeSerialSendText = new InvokeSerialSendText(SendTextToSerial);
            this.BeginInvoke(invokeSerialSendText, new Object[] { "POINT" });
            if (autoReadDataConfirmedEvent.WaitOne())
            {
                string pattern = @"The current write_pointer is:";
                string result = listString.Find(s => Regex.IsMatch(s, pattern));
                if (result != null)
                {
                    string[] parts = result.Split(':');
                    read_pointer = Convert.ToInt32(parts[1]);
                    this.BeginInvoke(invokeSerialSendText, new Object[] { "READ 0" });
                    if (autoReadDataConfirmedEvent.WaitOne())
                    {
                        listString.Clear();
                    }
                    for (int i = 0; i < read_pointer; i = i + 8)
                    {
                        this.BeginInvoke(invokeSerialSendText, new Object[] { "READ" });
                        if (autoReadDataConfirmedEvent.WaitOne())
                        {
                            string pattern2 = @"^\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2}";
                            listString = listString.FindAll(s => Regex.IsMatch(s, pattern2));
                            foreach (string s in listString)
                            {
                                count++;
                                string[] log_parts = s.Split(' ');
                                if (log_parts.Count() >= 4)
                                {
                                    LogLine line = new LogLine();
                                    line.index = count;
                                    line.time = Convert.ToDateTime(log_parts[0] + " " + log_parts[1]);
                                    line.temperature = Convert.ToSingle(log_parts[2]);
                                    line.moisture = Convert.ToSingle(log_parts[3]);
                                    listLogLine.Add(line);
                                    InvokeRefreshData invokeRefreshData = new InvokeRefreshData(refreshData);
                                    this.BeginInvoke(invokeRefreshData, new object[] {line.index, line.time, line.temperature, line.moisture});
                                }
                            }
                            listString.Clear();
                        }
                    }                    
                }                
            }
            this.BeginInvoke(invokeLockReadDataButton, new Object[] { false });
            InvokeAdjustChart invokeAdjustChart = new InvokeAdjustChart(adjustChart);
            this.BeginInvoke(invokeAdjustChart, new object[] {});
            
        }

        private void lockReadDataButton(bool bLockButton) 
        {

            ReadDataButton.Enabled = !bLockButton;
            buttonCleanSD.Enabled = !bLockButton;
        }

        private void refreshData(int index, DateTime time, float temperature, float moisture)
        {      
            chart1.Series[0].Points.AddXY(index, temperature);
            chart2.Series[0].Points.AddXY(index, moisture);
        }

        private void chart1_Click(object sender, EventArgs e)
        {
            adjustChart();
        }

        void adjustChart()
        {
            int min_temperature;
            int min_moisture;
            if (listLogLine == null || listLogLine.Count == 0)
            {
                min_temperature = 25;
                min_moisture = 50;
            }
            else
            {
                min_temperature = Convert.ToInt32(listLogLine[0].temperature);
                min_moisture = Convert.ToInt32(listLogLine[0].moisture);
            }
            min_temperature = listLogLine.Aggregate(min_temperature, (acc, x) => Math.Min(acc, Convert.ToInt32(x.temperature)));
            min_moisture = listLogLine.Aggregate(min_moisture, (acc, x) => Math.Min(acc, Convert.ToInt32(x.moisture)));
            //chart1.ChartAreas[0].AxisY.Minimum = min_temperature;
            //chart2.ChartAreas[0].AxisY.Minimum = min_moisture;
            label9.Text = "取回记录共计： " + listLogLine.Count + "条";

        }

        private void chart2_Click(object sender, EventArgs e)
        {
            adjustChart();
        }

        private void buttonCleanSD_Click(object sender, EventArgs e)
        {
            Thread threadSetClockAndClearSD = new Thread(new ThreadStart(setClockAndCleanSD));
            threadSetClockAndClearSD.Start();

        }

        private void debugToolStripMenuItem_Click(object sender, EventArgs e)
        {
            groupBoxSendData.Visible = !groupBoxSendData.Visible;
            textBoxReceive.Visible = !textBoxReceive.Visible;
            buttonSendData.Visible = !buttonSendData.Visible;
            buttonClearRecData.Visible = !buttonClearRecData.Visible;
        }
    }
}
