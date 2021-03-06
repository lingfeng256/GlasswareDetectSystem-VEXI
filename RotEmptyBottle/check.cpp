#include "check.h"
#include "HCPPGlobal.h"
#include "DlgMainWindow.h"
#include <string>
#include <fstream>
using namespace std;
#include "QTextStream"
#define MAX_LOGPATH_LENTH 255
#define DEBUG_MODE_LOG 1
//extern float fPressScale;	//应力图像增亮系数;

CCheck::CCheck(void)
{	
	m_dLastTime = 0.f;
	m_dTimeProcess = 0.f;
	m_pMainWnd = NULL;
	m_bContinueShow = false;
	m_bStopAtError = false;
	m_bExtExcludeDefect = false;
	m_bSaveErrorInfo = true;
	m_nBaseCharModelID = -1;
	m_vExcludeInfo.clear();

	// 2017.7-瓶底模点排序初始化
	strMoldOrderList.clear();
	strMoldOrderList<<"52121"<<"4111121"<<"4112111"<<"42221"<<"4211111"<<"42122"<<"3121121"<<"3122111"<<"3111221"<<"12125"
		<<"32123"<<"311111111"<<"3111122"<<"3112211"<<"3112112"<<"32321"<<"3221111"<<"32222"<<"113111111"<<"1211114"
		<<"111111113"<<"211111112"<<"3211211"<<"3211112"<<"2131121"<<"2132111"<<"2121221"<<"212111111"<<"111111311"<<"1112114"
		<<"2211113"<<"1121123"<<"22322"<<"2121122"<<"2122211"<<"2122112"<<"2111321"<<"211121111"<<"1132211"<<"12224"
		<<"1122113"<<"2111123"<<"2211212"<<"112111211"<<"2111222"<<"211111211"<<"2112311"<<"22421"<<"1122311"<<"1111124"
		<<"2112113"<<"1211312"<<"1122212"<<"2221112"<<"111131111"<<"2231111"<<"22212211"<<"2211311"<<"1121321"<<"22124"
		<<"12323"<<"1112312"<<"2112212"<<"112111112"<<"1111322"<<"12521"<<"1141121"<<"1142111"<<"1231211"<<"1211213"
		<<"1111223"<<"1221212"<<"1231112"<<"1132112"<<"1121222"<<"1211411"<<"111121211"<<"1131221"<<"1111421"<<"1112213"
		<<"22223"<<"111111212"<<"111121112"<<"12422"<<"1131122"<<"1112411"<<"1221311"<<"112121111"<<"1241111"<<"1221113";
	m_bIsChecking = false;
}
CCheck::~CCheck(void)
{
	m_vExcludeInfo.clear();
}
//*功能：拷贝构造函数，用于复制检测对象供主窗口调试
CCheck &CCheck::operator=(CCheck &cpCheck)
{
	strVision = cpCheck.strVision;				//算法版本
	strAppPath = cpCheck.strAppPath;			//系统路径
	strModelPath = cpCheck.strModelPath;		//模板路径
	vItemFlow = cpCheck.vItemFlow;				//检测流程
	vModelParas = cpCheck.vModelParas;			//模板参数
	vModelShapes = cpCheck.vModelShapes;		//模板形状
	m_ImageSrc = cpCheck.m_ImageSrc;			//原始图像	
	m_normalbotXld = cpCheck.m_normalbotXld;	//正常瓶子轮廓
	m_pressbotXld = cpCheck.m_pressbotXld;		//应力瓶子轮廓
	oRegError = cpCheck.oRegError;				//报错缺陷
	m_nWidth = cpCheck.m_nWidth;				//图像宽度
	m_nHeight = cpCheck.m_nHeight;				//图像高度
	m_nCamIndex = cpCheck.m_nCamIndex;			//相机序号
	modelOri = cpCheck.modelOri;				//模板原点
	currentOri = cpCheck.currentOri;			//当前原点
    m_nBaseCharModelID = cpCheck.m_nBaseCharModelID;
	normalOri = cpCheck.normalOri;				//正常图像原点，定位应力图像
	oldSideOri = cpCheck.oldSideOri;			//瓶身旧原点
	m_bLocate = cpCheck.m_bLocate;				//是否定位
	m_nLocateItemID = cpCheck.m_nLocateItemID;	//定位检测项ID
	m_bDisturb = cpCheck.m_bDisturb;			//是否排除干扰
	m_vDistItemID = cpCheck.m_vDistItemID;		//干扰区域检测项ID
	m_nMaxErrNum = cpCheck.m_nMaxErrNum;		//缺陷数量上限
	m_sCheckResult = cpCheck.m_sCheckResult;	//返回系统的结果
	rtnInfo = cpCheck.rtnInfo;					//返回算法的结果
	m_dLastTime = cpCheck.m_dLastTime;			//上一次检测完成时间
	m_dTimeProcess = cpCheck.m_dTimeProcess;	//当前检测耗时
	dLOFHei = cpCheck.dLOFHei;					//剪刀印检测时极坐标变换的图像高度，用于缺陷信息显示
	m_nColBodyCont = cpCheck.m_nColBodyCont;	//横向平移时瓶身轮廓用到
	m_dAngBodyCont = cpCheck.m_dAngBodyCont;	//横向平移时瓶身轮廓用到
	checkerAry = cpCheck.checkerAry;			//相机检测链表
	m_nCheckType = cpCheck.m_nCheckType;		//检测类型
	m_bSaveErrorInfo = cpCheck.m_bSaveErrorInfo;//保存错误信息（图像，模板等）,便于测试问题所在
	m_bIsChecking = cpCheck.m_bIsChecking;
	return *this;
}
//*功能：检测算法初始化
s_Status CCheck::init(s_AlgInitParam sAlgInitParam)
{
	s_Status sError;
	sError.nErrorID = RETURN_OK;
	strcpy_s(sError.chErrorInfo,"");
	strcpy_s(sError.chErrorContext,"");

	m_nCamIndex = sAlgInitParam.nCamIndex;//相机序号
	m_nCheckType = sAlgInitParam.nModelType;//检测类型
	m_nMaxErrNum = OUTPUT_MAXERRNUM;
	m_sCheckResult.vErrorParaAry = new s_ErrorPara[m_nMaxErrNum];
	for(int i = 0; i< m_nMaxErrNum; ++i)
	{
		m_sCheckResult.vErrorParaAry[i].nArea = 0;
		m_sCheckResult.vErrorParaAry[i].nLevel = 0;
		m_sCheckResult.vErrorParaAry[i].nErrorType = 0;
		m_sCheckResult.vErrorParaAry[i].rRctError.left = 0;
		m_sCheckResult.vErrorParaAry[i].rRctError.top = 0;
		m_sCheckResult.vErrorParaAry[i].rRctError.right = 0;
		m_sCheckResult.vErrorParaAry[i].rRctError.bottom = 0;
	}
	//2013.9.22 nanjc 初始化
	m_sCheckResult.sImgLocInfo.sLocOri.modelRow = 0;
	m_sCheckResult.sImgLocInfo.sLocOri.modelCol = 0;
	m_sCheckResult.sImgLocInfo.sLocOri.modelAngle = 0;
	m_sCheckResult.sImgLocInfo.sLocOri.modelRadius = 0;
	m_sCheckResult.sImgLocInfo.sXldPoint.nCount = 0;
	m_sCheckResult.sImgLocInfo.sXldPoint.nRowsAry = new int[BOTTLEXLD_POINTNUM]();
	m_sCheckResult.sImgLocInfo.sXldPoint.nColsAry = new int[BOTTLEXLD_POINTNUM]();

	if (QString::fromLocal8Bit(sAlgInitParam.chModelName).isEmpty())
	{
		sError.nErrorID = RETURN_CHECK_INIT;
		strcpy_s(sError.chErrorInfo,"模板名称为空!");
		strcpy_s(sError.chErrorContext,"CCheck-->init");
		return sError;
	}
	QDir modelDir;
	//获取系统路径
	getAppPath();
	//获取算法版本
	getVersion(strAppPath+"RotEmptyBottle.dll");
	//读入模板参数
	strModelPath = QString::fromLocal8Bit(sAlgInitParam.chCurrentPath)+"\\"+
		QString::fromLocal8Bit(sAlgInitParam.chModelName)+"\\"+
		QString::number(sAlgInitParam.nCamIndex);
	if (!modelDir.exists(strModelPath))
		modelDir.mkpath(strModelPath);	
	readModel(strModelPath);
	
	return sError;
}
s_Status CCheck::Free()
{
	s_Status sError;
	sError.nErrorID = RETURN_OK;
	strcpy_s(sError.chErrorInfo,"");
	strcpy_s(sError.chErrorContext,"");

	delete [] m_sCheckResult.vErrorParaAry;
	delete [] m_sCheckResult.sImgLocInfo.sXldPoint.nRowsAry;
	delete [] m_sCheckResult.sImgLocInfo.sXldPoint.nColsAry;
	return sError;
}
//*功能：检测主函数
s_Status CCheck::Check(s_AlgCInP sAlgCInP,s_AlgCheckResult **sAlgCheckResult)
{
	//sAlgCheckResult = NULL;//测试版本NULL == sAlgCheckResult|| 
	s_Status sError;
	sError.nErrorID = RETURN_NULL_PTR;
	/*if (NULL == *sAlgCheckResult)
		return sError;*/
	sError.nErrorID = RETURN_OK;
	strcpy_s(sError.chErrorInfo,"");
	strcpy_s(sError.chErrorContext,"");
	int i;
	Hlong nNum;
	HTuple Row1,Col1,Row2,Col2,Area;	
	double curTime,endTime;
	bool bUpdateImg = false;//是否允许连续显示时更新图像
	double dTimeInter = 0.1;//连续显示时间隔不小于500ms

	count_seconds(&curTime);
	if (curTime-m_dLastTime > dTimeInter || curTime < m_dLastTime)
	{
		m_dLastTime = curTime;
		bUpdateImg = true;
	}	

	gen_empty_obj(&oRegError);//报错缺陷
	gen_empty_obj(&rtnInfo.regError);//错误区域
	gen_empty_obj(&m_pressbotXld);//应力轮廓
	rtnInfo.nType=GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	rtnInfo.vExcludeInfo.clear();
	rtnInfo.nMouldID=0; //2017.8
	m_bLocate = false;
	m_bDisturb = false;
	m_sCheckResult.nSizeError = 0;	
	m_vExcludeInfo.clear();

	//2013.9.22 nanjc 接收正常图像原点,定位应力图像,不管何种模式都接收，用于修改定位方法或添加定位模块
	m_sCheckResult.sImgLocInfo.sXldPoint.nCount = 0;
	m_sCheckResult.sImgLocInfo.sLocOri.modelRow = 0;
	m_sCheckResult.sImgLocInfo.sLocOri.modelCol = 0;
	m_sCheckResult.sImgLocInfo.sLocOri.modelAngle = 0;
	m_sCheckResult.sImgLocInfo.sLocOri.modelRadius = 0;
	normalOri.Row = sAlgCInP.sImgLocInfo.sLocOri.modelRow;
	normalOri.Col = sAlgCInP.sImgLocInfo.sLocOri.modelCol;
	normalOri.Angle = sAlgCInP.sImgLocInfo.sLocOri.modelAngle;
	//2014.9.19 增加半径，暂从角度传入，修改系统接口+半径
	normalOri.Radius = sAlgCInP.sImgLocInfo.sLocOri.modelRadius;
	PtsToXLD(sAlgCInP.sImgLocInfo.sXldPoint,m_normalbotXld);
	//生成图像
	m_nWidth = sAlgCInP.sInputParam.nWidth;
	m_nHeight = sAlgCInP.sInputParam.nHeight;
	float fPressScale = sAlgCInP.fParam;
	if (fPressScale <= 0 || fPressScale>=10)
	{
		fPressScale = 1;
	}
	gen_image1_extern(&m_ImageSrc, "byte", m_nWidth, m_nHeight,(Hlong)(sAlgCInP.sInputParam.pcData), NULL);
	switch(m_nCheckType)
	{
	case CHECK_TYPE_ENHANCE://
		{
			mult_image(m_ImageSrc, m_ImageSrc,&m_ImageSrc,fPressScale,0);//g1 * g2 * Mult + Add
			char     typ[128];
			unsigned char *ptrSrc;
			get_image_pointer1(m_ImageSrc, (Hlong *)&ptrSrc, typ, NULL, NULL);
			memcpy(sAlgCInP.sInputParam.pcData, ptrSrc, sAlgCInP.sInputParam.nWidth*sAlgCInP.sInputParam.nHeight);
		}
		break;
	case CHECK_TYPE_ROTATE:
		{
			rotate_image(m_ImageSrc, &m_ImageSrc, sAlgCInP.nParam, "constant");
			char     typ[128];
			unsigned char *ptrSrc;
			get_image_pointer1(m_ImageSrc, (Hlong *)&ptrSrc, typ, NULL, NULL);
			memcpy(sAlgCInP.sInputParam.pcData, ptrSrc, sAlgCInP.sInputParam.nWidth*sAlgCInP.sInputParam.nHeight);
		}
		break;
	default:
		{
			//1. 分析流程(是否要定位，是否要排除干扰)
			mirror_image(m_ImageSrc, &m_ImageSrc, "row");
			m_vDistItemID.clear();
			for (i=0;i<vItemFlow.size();++i)
			{
				switch(vItemFlow[i])
				{
				case ITEM_SIDEWALL_LOCATE://side wall 侧壁瓶身定位
					m_nLocateItemID = i;
					if (vModelParas[i].value<s_pSideLoc>().bEnabled)
					{ 
						m_bLocate = true;
					}
					break;
				case ITEM_FINISH_LOCATE:
					m_nLocateItemID = i;
					if (vModelParas[i].value<s_pFinLoc>().bEnabled)
					{
						m_bLocate = true;
					}
					break;
				case ITEM_BASE_LOCATE:
					m_nLocateItemID = i;
					if (vModelParas[i].value<s_pBaseLoc>().bEnabled)
					{
						m_bLocate = true;
					}
					break;
				case ITEM_DISTURB_REGION:
					m_vDistItemID.push_back(i);
					m_bDisturb = true;
					break;
				default:
					break;
				}

				/*if (vItemFlow[i]==ITEM_SIDEWALL_LOCATE ||
				vItemFlow[i]==ITEM_FINISH_LOCATE ||
				vItemFlow[i]==ITEM_BASE_LOCATE)
				{
				m_nLocateItemID = i;
				m_bLocate = true;
				}						

				if (vItemFlow[i]==ITEM_DISTURB_REGION)
				{
				m_vDistItemID.push_back(i);
				m_bDisturb = true;
				}*/
			}
			//2. 处理定位及形状仿射变换
			if (m_bLocate)
			{
				Hobject bottleXld;
				try
				{
					//计算当前原点(正常定位时，提取瓶子轮廓)
					rtnInfo = fnFindCurrentPos(bottleXld);	
				}
				catch(HException &e)
				{
					//Halcon异常		
					QString tempStr,strErr;
					tempStr=e.message;
					tempStr.remove(0,20);
					strErr = QString("Locate abnormal:CCheck::Check::fnFindCurrentPos,")+ tempStr;//定位异常
					writeErrDataRecord(strErr);
					//2013.12.13 保存异常图像和模板
					if (m_bSaveErrorInfo)
					{
						m_bSaveErrorInfo = false;
						writeErrorInfo(strErr);
					}
					sError.nErrorID = RETURN_CHECK_ERROR;
					strcpy_s(sError.chErrorInfo,"Locate abnormal");//定位异常
					strcpy_s(sError.chErrorContext,"CCheck::Check::fnFindCurrentPos");
					return sError; 
				}
				catch (...)
				{
					writeErrDataRecord(QString("abnormal:CCheck::Check::fnFindCurrentPos,Locate abnormal"));//定位异常
					//2013.12.13 保存异常图像和模板
					if (m_bSaveErrorInfo)
					{
						m_bSaveErrorInfo = false;
						writeErrorInfo(QString("abnormal:CCheck::Check::fnFindCurrentPos,Locate abnormal"));////定位异常
					}

					sError.nErrorID = RETURN_CHECK_ERROR;
					strcpy_s(sError.chErrorInfo,"Locate abnormal");
					strcpy_s(sError.chErrorContext,"CCheck::Check::fnFindCurrentPos");
					return sError; 
				}
				//变换所有区域
				if (rtnInfo.nType==GOOD_BOTTLE)
				{
					if (modelOri.Row!=0 || modelOri.Col!=0)
					{
						affineAllShape();//防止新建模板时改变原点定位后面的区域
						//2014.12.5 变换区域才迭代原点，否则可能会导致区域漂移
						//模板原点更新为当前原点
						modelOri=currentOri;
					}					
					//2013.9.22 nanjc 传出正常图像定位参数
					m_sCheckResult.sImgLocInfo.sLocOri.modelRow = modelOri.Row;
					m_sCheckResult.sImgLocInfo.sLocOri.modelCol = modelOri.Col;
					m_sCheckResult.sImgLocInfo.sLocOri.modelAngle = modelOri.Angle;
					m_sCheckResult.sImgLocInfo.sLocOri.modelRadius = modelOri.Radius;
					XLDToPts(bottleXld,m_sCheckResult.sImgLocInfo.sXldPoint);
				}		
			}
			//3. 处理其余检测流程
			if (rtnInfo.nType==GOOD_BOTTLE)
			{
				try
				{
					for (i=0;i<vItemFlow.size();++i)
					{
						RtnInfo tempRtn;
						switch(vItemFlow[i])
						{
						case ITEM_HORI_SIZE:
							tempRtn = fnHoriSize(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_VERT_SIZE:
							tempRtn = fnVertSize(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_FULL_HEIGHT:
							tempRtn = fnFullHeight(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;			
						case ITEM_BENT_NECK:
							tempRtn = fnBentNeck(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_VERT_ANG:
							tempRtn = fnVertAng(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SGENNERAL_REGION:
							tempRtn = fnSGenReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SSIDEFINISH_REGION:
							tempRtn = fnSSideFReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SINFINISH_REGION:
							tempRtn = fnSInFReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SSCREWFINISH_REGION:
							tempRtn = fnSScrewFReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_FRLINNER_REGION:
							tempRtn = fnFRLInnerReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_FRLMIDDLE_REGION:
							tempRtn = fnFRLMiddleReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_FRLOUTER_REGION:
							tempRtn = fnFRLOuterReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_FBLINNER_REGION:
							tempRtn = fnFBLInnerReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_FBLMIDDLE_REGION:
							tempRtn = fnFBLMiddleReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_BINNER_REGION:
							tempRtn = fnBInnerReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_BMIDDLE_REGION:
							tempRtn = fnBMiddleReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_BOUTER_REGION:
							tempRtn = fnBOutterReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SSIDEWALL_REGION:
							tempRtn = fnSSidewallReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
							/*case ITEM_SSIDE_CRACK:
							tempRtn = fnSSidewallReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;*/
						case ITEM_FINISH_CONTOUR:
							tempRtn = fnFinishCont(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_NECK_CONTOUR:
							tempRtn = fnNeckCont(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_BODY_CONTOUR:
							tempRtn = fnBodyCont(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SBRISPOT_REGION:
							tempRtn = fnSBriSpotReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_BALL_REGION:
							//tempRtn = fnBModeNOReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							tempRtn = fnBMouldNOReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);							
							break;
						case ITEM_CIRCLE_SIZE:
							tempRtn = fnCircleSize(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SBASE_REGION:
							tempRtn = fnSBaseReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						case ITEM_SBASECONVEX_REGION:
							tempRtn = fnSBaseConvexReg(m_ImageSrc,vModelParas[i],vModelShapes[i]);
							break;
						default:
							break;
						}
						//2013.12.18将提取的排除缺陷添加到m_vExcludeInfo中
						if (m_bExtExcludeDefect && tempRtn.vExcludeInfo.count()>0)
						{
							m_vExcludeInfo.append(tempRtn.vExcludeInfo);
						}
						if (tempRtn.nType>0)
						{
							rtnInfo.nItem = i;//用于通知检测项风格变化
							rtnInfo.nType = tempRtn.nType;
							rtnInfo.strErrorNm = tempRtn.strErrorNm; 
							writeAlgLog(tempRtn.strErrorNm);
							rtnInfo.strEx = tempRtn.strEx;       //2017.7						
							concat_obj(rtnInfo.regError,tempRtn.regError,&rtnInfo.regError);
						}
						//2017.8-不论是否报错，返回识别模号
						rtnInfo.nMouldID = tempRtn.nMouldID; 
					}	

					//过滤排除缺陷
					FilterExcludeDefect(rtnInfo);
				}
				catch(HException &e)
				{
					//Halcon异常		
					QString tempStr,strErr;
					tempStr=QString("%1%2%3%4%5").arg(e.message).arg(",,").arg(e.file).arg(",,").arg(e.line);
					//tempStr.remove(0,20);
					strErr = QString("Inspection module abnormal:CCheck::Check,")+ tempStr;//检测模块异常
					writeErrDataRecord(strErr);
					//2013.12.13 保存异常图像和模板
					if (m_bSaveErrorInfo)
					{
						m_bSaveErrorInfo = false;
						writeErrorInfo(strErr);
					}
					sError.nErrorID = RETURN_CHECK_ERROR;
					strcpy_s(sError.chErrorInfo,"Inspection module abnormal");//检测模块异常
					strcpy_s(sError.chErrorContext,"CCheck::Check");
					return sError; 
				}
				catch (...)
				{
					writeErrDataRecord(QString("abnormal:CCheck::Check,Inspection module abnormal"));//检测模块异常
					//2013.12.13 保存异常图像和模板
					if (m_bSaveErrorInfo)
					{
						m_bSaveErrorInfo = false;
						writeErrorInfo(QString("abnormal:CCheck::Check,Inspection module abnormal"));//检测模块异常
					}
					sError.nErrorID = RETURN_CHECK_ERROR;
					strcpy_s(sError.chErrorInfo,"Inspection module abnormal");//检测模块异常
					strcpy_s(sError.chErrorContext,"CCheck::Check");
					return sError; 
				}
			}	
			//主窗口连续显示
			if (m_bContinueShow && bUpdateImg)
			{
				copy_image(m_ImageSrc,&m_pMainWnd->imgShow);
				//2013.9.24 nanjc 应力定位
				m_pMainWnd->bottleOri = normalOri;
				copy_obj(m_normalbotXld,&m_pMainWnd->bottleContour,1,-1);
				int nType = m_pMainWnd->check(m_pMainWnd->imgShow,m_pMainWnd->bottleOri,m_pMainWnd->bottleContour);		
				//clear_window(m_pMainWnd->m_lWindID);//不清空会造成halcon窗口颜色设置无效、绘图报错等异常!
				m_pMainWnd->displayObject();		
				if (m_bStopAtError && nType != GOOD_BOTTLE)
				{
					m_bContinueShow = m_bStopAtError = false;	
					//2013.9.12 nanjc 发送停止连续显示信号
					emit signals_stopContinue();
				}
			}	
			//4. 将检测结果返回给系统
			if (rtnInfo.nType>0)
			{
				//int debug = 0;  //wait more test。maybe add a config parameter
				copy_obj(rtnInfo.regError,&oRegError,1,-1);
				union1(oRegError,&oRegError);
				connection(oRegError,&oRegError);
				count_obj(oRegError,&nNum);
				nNum = nNum>m_nMaxErrNum?m_nMaxErrNum:nNum;
				m_sCheckResult.nSizeError =  nNum ; 
				
				if(0 == m_sCheckResult.nSizeError) //protect for num is 0
				     m_sCheckResult.nSizeError = 1;	
				
				smallest_rectangle1(oRegError, &Row1, &Col1, &Row2, &Col2);
				area_center(oRegError, &Area, NULL, NULL);
				for (int i=0; i < nNum; i++)
				{				
					m_sCheckResult.vErrorParaAry[i].nErrorType = rtnInfo.nType;
					m_sCheckResult.vErrorParaAry[i].nLevel = 1;
					m_sCheckResult.vErrorParaAry[i].nArea= Area[i].L();
					m_sCheckResult.vErrorParaAry[i].rRctError.left = Col1[i].L();
					m_sCheckResult.vErrorParaAry[i].rRctError.top = Row1[i].L();
					m_sCheckResult.vErrorParaAry[i].rRctError.right = Col2[i].L();
					m_sCheckResult.vErrorParaAry[i].rRctError.bottom = Row2[i].L();
					m_sCheckResult.vErrorParaAry[i].strErrorNm = rtnInfo.strErrorNm;
					writeAlgLog(rtnInfo.strErrorNm);//QString::fromLocal8Bit(
					AdaptRect(m_sCheckResult.vErrorParaAry[i].rRctError, m_nWidth, m_nHeight);
				}
			}
			m_sCheckResult.nMouldID = rtnInfo.nMouldID;
			*sAlgCheckResult = &m_sCheckResult;
		}
		break;
	}
	count_seconds(&endTime);
	m_dTimeProcess = (endTime-curTime)*1000;
	return sError;
}
s_Status CCheck::CopySizePara(s_SizePara4Copy &sSizePara)
{
	s_Status sError;
	sError.nErrorID = RETURN_OK;
	strcpy_s(sError.chErrorInfo,"");
	strcpy_s(sError.chErrorContext,"");

	return sError;
}

//*功能
bool CCheck::setsSystemInfo(s_SystemInfoforAlg &sSystemInfo)
{
	m_bIsChecking = sSystemInfo.bIsChecking; //系统检测状态
	return true;
}

//*功能：返回一条线与瓶身的两个交点
bool CCheck::FindTwoPointsSubpix(Alg::s_ImagePara sImagePara,double inRow,POINT &pLeft,POINT &pRight,int nEdge)
{
	Hobject imgShow,LineSeg;
	Hobject RegGen, ImgReduce, Edges, Xld1, Xld2;
	Hlong nNum = 0;
	Hlong nCount = 0;
	HTuple Rows, Cols;
	double RowMean, ColMean;

	// 清空目标点
	pLeft.x=0;
	pLeft.y=0;
	pRight.x=0;
	pRight.y=0;

	// 生成图像
	gen_image1(&imgShow, "byte", sImagePara.nWidth, sImagePara.nHeight,(Hlong)sImagePara.pcData);
	mirror_image(imgShow, &imgShow, "row");
	gen_region_line(&LineSeg, inRow, 0, inRow, sImagePara.nWidth-1);
	dilation_circle(LineSeg, &RegGen, 3.5);
	reduce_domain(imgShow, RegGen, &ImgReduce);
	edges_sub_pix(ImgReduce, &Edges, "sobel_fast", 1, nEdge, (2*nEdge));
	select_contours_xld(Edges, &Edges, "contour_length", 3, 99999, 0, 0);
	select_shape_xld(Edges, &Edges, "height","and", 4, 99999);
	count_obj(Edges, &nNum);

	if (nNum < 2)
	{
		nCount = 0;
	}
	else if (nNum > 1)
	{
		// 选择左边点
		sort_contours_xld(Edges, &Edges, "upper_left", "true", "column");
		select_obj(Edges, &Xld1, 1);
		get_contour_xld(Xld1, &Rows, &Cols);
		tuple_mean(Rows, &RowMean);
		tuple_mean(Cols, &ColMean);
		pLeft.x=ColMean;
		pLeft.y=RowMean;

		// 选择右边点
		select_obj(Edges, &Xld2, nNum);
		get_contour_xld(Xld2, &Rows, &Cols);
		tuple_mean(Rows, &RowMean);
		tuple_mean(Cols, &ColMean);
		pRight.x=ColMean;
		pRight.y=RowMean;

		nCount = 2;
	}

	if (nCount !=2)
	{
		return false;
	}
	return true;
}

//*功能：获取执行路径
void CCheck::getAppPath()
{
	//因为Qt是UNICODE，GetModuXXX第一个参数需要LPWSTR类型
	TCHAR tcStr[MAX_PATH];
	//_splitpath第一个参数需要LPCSTR类型
	char cStr[MAX_PATH];
	char drive[MAX_PATH], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	GetModuleFileName(NULL, tcStr, sizeof(tcStr));
	//需要把宽字节tcStr转换为多字节cStr
	int iLength ;  	
	iLength = WideCharToMultiByte(CP_ACP, 0, tcStr, -1, NULL, 0, NULL, NULL); 	
	WideCharToMultiByte(CP_ACP, 0, tcStr, -1, cStr, iLength, NULL, NULL); 
	//拆分
	_splitpath_s(cStr, drive, dir, fname, ext);
	strcat_s(drive, dir);
	//多字节再转换为UNICODE，以上可防止中文路径乱码
	strAppPath = QString::fromLocal8Bit(drive);	
}
//*功能：获取算法版本号
void CCheck::getVersion(QString strFullName)
{
	//将QString 转换为LPCWSTR
	const wchar_t * encodedName = reinterpret_cast<const wchar_t *>(strFullName.utf16());
	int cbInfoSize=::GetFileVersionInfoSize(encodedName, NULL);   
	LPVOID Version = new char[cbInfoSize];  
	UINT cbVerSize;
	GetFileVersionInfo(encodedName, NULL, cbInfoSize, Version);
	int ma, mj, r, b;
	VS_FIXEDFILEINFO *VInfo;


	int a=::VerQueryValueW(Version,TEXT("\\"), ((void**)&VInfo), (PUINT)&cbVerSize);
	ma= VInfo->dwFileVersionMS >> 16;
	mj= VInfo->dwFileVersionMS & 0x00ff;
	r= VInfo->dwFileVersionLS >> 16;
	b= VInfo->dwFileVersionLS & 0x00ff;

	strVision=QString("%1.%2.%3.%4").arg(ma).arg(mj).arg(r).arg(b);
	delete []Version;

}
//*功能：记录异常日志 LogFiles\\AlgorithmDll\\2017-8-7.txt
void CCheck::writeErrDataRecord(QString strData)
{
	QDate date = QDate::currentDate();
	QTime time = QTime::currentTime();

	QString strFilePath = strAppPath+"LogFiles\\AlgorithmDll\\"+
		QString("%1-%2-%3.txt").arg(date.year()).arg(date.month()).arg(date.day());
	QSettings errorSet(strFilePath,QSettings::IniFormat);
	//QApplication::addLibraryPath("./QtPlugins");
	errorSet.setIniCodec("GBK");
	//errorSet.setIniCodec("utf-8");
	errorSet.beginGroup("setup");
	int nTotalNum = errorSet.value("total",0).toInt();
	nTotalNum = nTotalNum>1000?0:++nTotalNum;
	errorSet.endGroup();
	errorSet.beginGroup("ErrorData");
	QString errorData = QString("%1-%2-%3 %4:%5:%6 %7").arg(date.year()).arg(date.month()).
		arg(date.day()).arg(time.hour()).arg(time.minute()).arg(time.second()).arg(strData);
	errorSet.setValue(QString("Error_%1").arg(nTotalNum),errorData);
	errorSet.endGroup();
	errorSet.beginGroup("setup");
	errorSet.setValue("total",QString::number(nTotalNum));
	errorSet.endGroup();	
	//补充：日期和时间数字前面补0，对齐
}
//*功能：保存错误信息 LogFiles\\AlgorithmDll\\2017-8-7\\ErrorInfo.txt
void CCheck::writeErrorInfo(QString strData)
{
	QDate date = QDate::currentDate();
	QTime time = QTime::currentTime();

	QString strFilePath = strAppPath+"LogFiles\\AlgorithmDll\\"+
		QString("%1-%2-%3").arg(date.year()).arg(date.month()).arg(date.day())+
		"\\ErrorInfo.txt";
	QSettings errorSet(strFilePath,QSettings::IniFormat);
	//QApplication::addLibraryPath("./QtPlugins");
	errorSet.setIniCodec("GBK");
	//errorSet.setIniCodec("utf-8");
	errorSet.beginGroup("setup");
	int nTotalNum = errorSet.value("total",0).toInt();
	nTotalNum = nTotalNum>1000?0:++nTotalNum;
	errorSet.endGroup();
	errorSet.beginGroup("ErrorInfo");
	QString errorData = QString("%1-%2-%3 %4:%5:%6 %7").arg(date.year()).arg(date.month()).
		arg(date.day()).arg(time.hour()).arg(time.minute()).arg(time.second()).arg(strData);
	errorSet.setValue(QString("Error_%1").arg(nTotalNum),errorData);
	errorSet.endGroup();
	errorSet.beginGroup("setup");
	errorSet.setValue("total",QString::number(nTotalNum));
	errorSet.endGroup();
	//保存错误文件
	QString strErrorFileDir = strAppPath+"LogFiles\\AlgorithmDll\\"+
		QString("%1-%2-%3").arg(date.year()).arg(date.month()).arg(date.day())+"\\"+
		QString("Error_%1").arg(nTotalNum)+"\\";
	QDir modelDir;
	if (!modelDir.exists(strErrorFileDir))
		modelDir.mkpath(strErrorFileDir);	
	strFilePath = strErrorFileDir + QString("Cam_%1").arg(m_nCamIndex+1);
	//保存错误图像
	write_image(m_ImageSrc, "bmp", 0, strFilePath.toLocal8Bit().constData());
	//保存错误模板
	QString strModel = strModelPath+"\\ModelPara.ini";
	QString strCopy = strErrorFileDir +"\\ModelPara.ini";
	if (QFile::exists(strCopy))
	{
		QFile::remove(strCopy);
	}
	QFile::copy(strModel,strCopy);
	strModel = strModelPath+"\\ModelShape.ini";
	strCopy = strErrorFileDir +"\\ModelShape.ini";
	if (QFile::exists(strCopy))
	{
		QFile::remove(strCopy);
	}
	QFile::copy(strModel,strCopy);	
}
void CCheck::writeAlgLogLx(char *acLoginfo)
{
    char tmpLog[MAX_LOGPATH_LENTH]=".\\lxnt.txt";
	 
	FILE *fp=fopen(tmpLog,"a+");
	fprintf(fp,acLoginfo);
	fprintf(fp,"\n");
	fclose(fp);
}
//*功能：记录算法操作日志 LogFiles\\AlgorithmDll\\AlgOperation\\2017-8-7.txt
void CCheck::writeAlgOperationRecord(QString strData)
{
	QDate date = QDate::currentDate();
	QTime time = QTime::currentTime();
	QString strFilePath = strAppPath+"LogFiles\\AlgorithmDll\\AlgOperation\\"+
		QString("%1-%2-%3.txt").arg(date.year()).arg(date.month()).arg(date.day());
	QSettings errorSet(strFilePath,QSettings::IniFormat);
	errorSet.setIniCodec("GBK");
	errorSet.beginGroup("setup");
	int nTotalNum = errorSet.value("total",0).toInt();
	nTotalNum = nTotalNum>1000?0:++nTotalNum;
	errorSet.endGroup();
	errorSet.beginGroup("ErrorData");
	QString errorData = QString("%1-%2-%3 %4:%5:%6 ").arg(date.year()).arg(date.month()).
		arg(date.day()).arg(time.hour()).arg(time.minute()).arg(time.second())+strData;
	errorSet.setValue(QString("Error_%1").arg(nTotalNum),errorData);//.toLocal8Bit().data()
	errorSet.endGroup();
	errorSet.beginGroup("setup");
	errorSet.setValue("total",QString::number(nTotalNum));
	errorSet.endGroup();	
	//补充：日期和时间数字前面补0，对齐
}

void CCheck::readModel(QString strPath)
{
	int i;	
	QString strParaPath = strPath+"\\ModelPara.ini";
	QSettings paraSet(strParaPath,QSettings::IniFormat);
	//QApplication::addLibraryPath("./QtPlugins");
	paraSet.setIniCodec("GBK");
	QString strShapePath = strPath+"\\ModelShape.ini";
	QSettings shapeSet(strShapePath,QSettings::IniFormat);

	////瓶底字符定位匹配模板
	//QString strCharModel = strPath+"\\CharModel.shm";
	//QByteArray ba = strCharModel.toLatin1();
	//const char *pCharModel = ba.data();
	//long isExist;

	vItemFlow.clear();
	vModelParas.clear();
	vModelShapes.clear();

	if (paraSet.childGroups().size()==0)
		return;//模板为空

	//2013.12.31 nanjc 修改：childGroups()获取的QSettings自动排序，当超过10个检测项时，
	//2013.12.31 nanjc 修改：10，11 将排在2的前面，导致检测项混乱。
	//按参数顺序读取
	//	foreach(QString group,paraSet.childGroups())
	for (i = 0;i<paraSet.childGroups().size();++i)
	{
		paraSet.beginGroup("Item_"+QString::number(i+1));
		shapeSet.beginGroup("Item_"+QString::number(i+1));
		int nType = paraSet.value("type").toInt();
		vItemFlow.push_back(nType);//压入检测流程
		vModelParas.push_back(readParabyType(nType,paraSet));//压入参数数据	
		vModelShapes.push_back(readShapebyType(nType,shapeSet));//压入形状数据
		paraSet.endGroup();
		shapeSet.endGroup();
	}	
	//更新模板原点
	for (i=0;i<vItemFlow.size();++i)
	{
		if (vItemFlow[i] == ITEM_SIDEWALL_LOCATE)
		{
			modelOri.Row =vModelShapes[i].value<s_oSideLoc>().ori.Row;
			modelOri.Col =vModelShapes[i].value<s_oSideLoc>().ori.Col;
			modelOri.Angle =vModelShapes[i].value<s_oSideLoc>().ori.Angle;
			oldSideOri = vModelShapes[i].value<s_oSideLoc>().ori;//用于自动纠错
		}
		if (vItemFlow[i] == ITEM_FINISH_LOCATE)
		{
			modelOri.Row =vModelShapes[i].value<s_oFinLoc>().Row;
			modelOri.Col =vModelShapes[i].value<s_oFinLoc>().Col;
		}
		if (vItemFlow[i] ==ITEM_BASE_LOCATE)
		{
			modelOri.Row =vModelShapes[i].value<s_oBaseLoc>().Row;
			modelOri.Col =vModelShapes[i].value<s_oBaseLoc>().Col;
			modelOri.Radius =vModelShapes[i].value<s_oBaseLoc>().Radius;
			modelOri.Angle =vModelShapes[i].value<s_oBaseLoc>().Angle;
			if(9 == vModelShapes[i].value<s_pBaseLoc>().nMethodIdx)//形状定位
			{	
				Hlong isExist;
				file_exists(".\\model_nut.shm", &isExist);
				char aLogInfo[MAX_LOGPATH_LENTH];
			    if(isExist == TRUE)
				{
					s_oBaseLoc oBaLoc = vModelShapes[i].value<s_oBaseLoc>();
					read_shape_model( ".\\model_nut.shm",&oBaLoc.ModelID);
					oBaLoc.ifGenSp = true;
					vModelShapes[i].setValue(oBaLoc);
				}
				
			}
		}
		if (vItemFlow[i] == ITEM_BINNER_REGION)
		{
			//瓶底字符定位匹配模板
			QString strCharModel = strPath+QString("\\CharModel_%1.shm").arg(i);
			QByteArray ba = strCharModel.toLatin1();
			const char *pCharModel = ba.data();
			Hlong isExist;

			file_exists(pCharModel, &isExist);
			if (isExist == TRUE)
			{
				read_shape_model(pCharModel, &m_nBaseCharModelID);
				s_oBInReg oBInReg = vModelShapes[i].value<s_oBInReg>();
				oBInReg.ModelID = m_nBaseCharModelID;
				vModelShapes[i].setValue(oBInReg);
			}
		}
	}	
}
//*功能：根据检测项的类型，读入检测项对应结构体的参数数据,如配置文件内没有参数，则返回默认值
QVariant CCheck::readParabyType(int nType,QSettings &set)
{
	QVariant vItemPara;
	switch(nType)
	{
	case ITEM_SIDEWALL_LOCATE:
		{
			s_pSideLoc pSideLoc;
			pSideLoc.strName = set.value("name",tr("Body Locate")).toString();
			pSideLoc.nType = set.value("type",ITEM_SIDEWALL_LOCATE).toInt();
			pSideLoc.bEnabled = set.value("bEnabled",true).toBool();
			pSideLoc.nMethodIdx = set.value("nMethodIdx",0).toInt();
			pSideLoc.bStress = set.value("bStress",false).toBool();
			pSideLoc.nFloatRange = set.value("nFloatRange",0).toInt();
			//pSideLoc.nLeftRight = set.value("nLeftRight",1).toInt();
			pSideLoc.nDirect = set.value("nDirect",0).toInt();
			pSideLoc.nEdge = set.value("nEdge",10).toInt();
			pSideLoc.bFindPointSubPix = set.value("bFindPointSubPix",false).toBool();
			vItemPara=QVariant::fromValue(pSideLoc);
		}
		break;
	case ITEM_FINISH_LOCATE:
		{
			s_pFinLoc pFinLoc;
			pFinLoc.strName = set.value("name",tr("Mouth Locate")).toString();
			pFinLoc.nType = set.value("type",ITEM_FINISH_LOCATE).toInt();
			pFinLoc.bEnabled = set.value("bEnabled",true).toBool();
			pFinLoc.nMethodIdx = set.value("nMethodIdx",0).toInt();
			pFinLoc.nEdge = set.value("nEdge",5).toInt();
			pFinLoc.fOpenSize = set.value("fOpenSize",1.5).toFloat();
			pFinLoc.nLowThres = set.value("nLowThres",5).toInt();
			pFinLoc.nHighThres = set.value("nHighThres",10).toInt();
			pFinLoc.nCenOffset = set.value("nCenOffset",200).toInt();
			pFinLoc.nFloatRange = set.value("nFloatRange",20).toInt();
			vItemPara=QVariant::fromValue(pFinLoc);
		}
		break;
	case ITEM_BASE_LOCATE:
		{
			s_pBaseLoc pBaseLoc;
			pBaseLoc.strName = set.value("name",tr("Bottom Locate")).toString();
			pBaseLoc.nType = set.value("type",ITEM_BASE_LOCATE).toInt();
			pBaseLoc.bEnabled = set.value("bEnabled",true).toBool();
			pBaseLoc.bRectShape = set.value("checkRectShape",false).toBool();
			pBaseLoc.nMethodIdx = set.value("nMethodIdx",0).toInt();
			pBaseLoc.bIgnore = set.value("bIgnore",false).toBool();
			pBaseLoc.fSegRatio = set.value("fSegRatio",0.75).toFloat();
			pBaseLoc.nGray = set.value("nGray",20).toInt();
			pBaseLoc.nEdge = set.value("nEdge",10).toInt();
			pBaseLoc.nRadiusOffset = set.value("nRadiusOffset",30).toInt();
			pBaseLoc.nBeltSpace = set.value("nBeltSpace",25.0).toInt();
			pBaseLoc.nBeltWidth = set.value("nBeltWidth",10).toInt();
			pBaseLoc.nBeltHeight = set.value("nBeltHeight",20).toInt();
		    pBaseLoc.nBeltEdge = set.value("nBeltEdge",5).toInt();
			pBaseLoc.nMoveOffset = set.value("nMoveOffset",20).toInt();
			pBaseLoc.nMoveStep = set.value("nMoveStep",2).toInt();
			vItemPara=QVariant::fromValue(pBaseLoc);
		}
		break;
	case ITEM_HORI_SIZE:
		{
			s_pHoriSize pHoriLoc;
			pHoriLoc.strName = set.value("name",tr("Hor Size")).toString();
			pHoriLoc.nType = set.value("type",ITEM_HORI_SIZE).toInt();
			pHoriLoc.bEnabled = set.value("bEnabled",true).toBool();

			pHoriLoc.bComplex = set.value("bComplex",false).toBool();
			pHoriLoc.nSizeGroup = set.value("nSizeGroup",1).toInt();
			pHoriLoc.nEdge = set.value("nEdge",10).toInt();
			pHoriLoc.fCurValue = set.value("fCurValue",0).toFloat();
			pHoriLoc.fLower = set.value("fLower",0).toFloat();
			pHoriLoc.fUpper = set.value("fUpper",999.9).toFloat();
			pHoriLoc.fReal = set.value("fReal",0).toFloat();
			pHoriLoc.fModify = set.value("fModify",0.3).toFloat();
			pHoriLoc.fRuler = set.value("fRuler",1).toFloat();
			vItemPara=QVariant::fromValue(pHoriLoc);
		}
		break;
	case ITEM_VERT_SIZE:
		{
			s_pVertSize pVertLoc;
			pVertLoc.strName = set.value("name",tr("Vert Size")).toString();
			pVertLoc.nType = set.value("type",ITEM_VERT_SIZE).toInt();
			pVertLoc.bEnabled = set.value("bEnabled",true).toBool();

			pVertLoc.bComplex = set.value("bComplex",false).toBool();
			pVertLoc.nSizeGroup = set.value("nSizeGroup",1).toInt();
			pVertLoc.nEdge = set.value("nEdge",10).toInt();
			pVertLoc.fCurValue = set.value("fCurValue",0).toFloat();
			pVertLoc.fLower = set.value("fLower",0).toFloat();
			pVertLoc.fUpper = set.value("fUpper",999.9).toFloat();
			pVertLoc.fReal = set.value("fReal",0).toFloat();
			pVertLoc.fModify = set.value("fModify",0.3).toFloat();
			pVertLoc.fRuler = set.value("fRuler",1).toFloat();
			vItemPara=QVariant::fromValue(pVertLoc);
		}
		break;
	case ITEM_FULL_HEIGHT:
		{
			s_pFullHeight pFullHeight;
			pFullHeight.strName = set.value("name",tr("Full Height")).toString();
			pFullHeight.nType = set.value("type",ITEM_FULL_HEIGHT).toInt();
			pFullHeight.bEnabled = set.value("bEnabled",true).toBool();

			//pFullHeight.bComplex = set.value("bComplex",false).toBool();
			//pFullHeight.nSizeGroup = set.value("nSizeGroup",1).toInt();
			pFullHeight.nEdge = set.value("nEdge",10).toInt();
			pFullHeight.fCurValue = set.value("fCurValue",0).toFloat();
			pFullHeight.fLower = set.value("fLower",0).toFloat();
			pFullHeight.fUpper = set.value("fUpper",9999.9).toFloat();
			pFullHeight.fReal = set.value("fReal",0).toFloat();
			pFullHeight.fModify = set.value("fModify",0.3).toFloat();
			pFullHeight.fRuler = set.value("fRuler",1).toFloat();
			vItemPara=QVariant::fromValue(pFullHeight);
		}
		break;
	case ITEM_BENT_NECK:
		{
			s_pBentNeck pBentNeck;
			pBentNeck.strName = set.value("name",tr("Bent Neck")).toString();
			pBentNeck.nType = set.value("type",ITEM_BENT_NECK).toInt();
			pBentNeck.bEnabled = set.value("bEnabled",true).toBool();
			pBentNeck.nBentNeck = set.value("nBentNeck",10).toInt();
			vItemPara=QVariant::fromValue(pBentNeck);
		}
		break;
	case ITEM_VERT_ANG:
		{
			s_pVertAng pVertAng;
			pVertAng.strName = set.value("name",tr("Vert Angle")).toString();
			pVertAng.nType = set.value("type",ITEM_VERT_ANG).toInt();
			pVertAng.bEnabled = set.value("bEnabled",true).toBool();
			pVertAng.fRuler = set.value("fRuler",90).toFloat();
			pVertAng.fVertAng = set.value("fVertAng",5).toFloat();
			vItemPara=QVariant::fromValue(pVertAng);
		}
		break;
	case ITEM_SGENNERAL_REGION:
		{
			s_pSGenReg pSGenReg;
			pSGenReg.strName = set.value("name",tr("Body Gen Region")).toString();
			pSGenReg.nType = set.value("type",ITEM_SGENNERAL_REGION).toInt();
			pSGenReg.bEnabled = set.value("bEnabled",true).toBool();
			pSGenReg.nShapeType = set.value("nShapeType",0).toInt();
			pSGenReg.nEdge = set.value("nEdge",15).toInt();
			pSGenReg.nGray = set.value("nGray",30).toInt();
			pSGenReg.nArea = set.value("nArea",100).toInt();
			pSGenReg.nLength = set.value("nLength",30).toInt();
			pSGenReg.bStone = set.value("bStone",false).toBool();
			pSGenReg.nStoneEdge = set.value("nStoneEdge",50).toInt();
			pSGenReg.nStoneArea = set.value("nStoneArea",80).toInt();
			pSGenReg.nStoneNum =  set.value("nStoneNum",1).toInt();
			pSGenReg.bDarkdot = set.value("bDarkdot",false).toBool();
			pSGenReg.nDarkdotEdge = set.value("nDarkdotEdge",65).toInt();
			pSGenReg.nDarkdotArea = set.value("nDarkdotArea",70).toInt();
			pSGenReg.fDarkdotCir = set.value("fDarkdotCir",0.3).toFloat();
			pSGenReg.nDarkdotNum = set.value("nDarkdotNum",1).toInt();
			pSGenReg.bTinyCrack = set.value("bTinyCrack",false).toBool();
			pSGenReg.nTinyCrackEdge = set.value("nTinyCrackEdge",10).toInt();
			pSGenReg.nTinyCrackAnsi = set.value("nTinyCrackAnsi",5).toInt();
			pSGenReg.nTinyCrackLength = set.value("nTinyCrackLength",0).toInt();
			pSGenReg.nTinyCrackInRadius = set.value("nTinyCrackInRadius",5).toInt();
			pSGenReg.nTinyCrackPhiL = set.value("nTinyCrackPhiL",0).toInt();
			pSGenReg.nTinyCrackPhiH = set.value("nTinyCrackPhiH",30).toInt();
			pSGenReg.bLightStripe = set.value("bLightStripe",false).toBool();
			pSGenReg.nLightStripeEdge = set.value("nLightStripeEdge",30).toInt();
			pSGenReg.nLightStripeLength = set.value("nLightStripeLength",30).toInt();
			pSGenReg.nLightStripeInRadius = set.value("nLightStripeInRadius",3).toInt();
			pSGenReg.nLightStripePhiL = set.value("nLightStripePhiL",70).toInt();
			pSGenReg.nLightStripePhiH = set.value("nLightStripePhiH",110).toInt();
			pSGenReg.bBubbles = set.value("bBubbles",false).toBool();
			pSGenReg.nBubblesLowThres = set.value("nBubblesLowThres",10).toInt();
			pSGenReg.nBubblesHighThres = set.value("nBubblesHighThres",20).toInt();
			pSGenReg.fBubblesCir = set.value("fBubblesCir",0.2).toFloat();
			pSGenReg.nBubblesLength = set.value("nBubblesLength",50).toInt();
			//pSGenReg.nBubblesArea = set.value("nBubblesArea",70).toInt();
			//pSGenReg.nBubblesGrayOffset = set.value("nBubblesGrayOffset",10).toInt();

			pSGenReg.bDistEdge = set.value("bDistEdge",false).toBool();
			pSGenReg.nDistEdge = set.value("nDistEdge",10).toInt();
			pSGenReg.bDistCon1 = set.value("bDistCon1",false).toBool();
			//pSGenReg.nDistPhiL1 = set.value("nDistPhiL1",85).toInt();
			//pSGenReg.nDistPhiH1 = set.value("nDistPhiH1",95).toInt();
			pSGenReg.nDistVerPhi = set.value("nDistVerPhi",10).toInt();
			pSGenReg.nDistAniL1 = set.value("nDistAniL1",3).toInt();
			pSGenReg.nDistAniH1 = set.value("nDistAniH1",999).toInt();
			pSGenReg.nDistInRadiusL1 = set.value("nDistInRadiusL1",0).toInt();
			pSGenReg.nDistInRadiusH1 = set.value("nDistInRadiusH1",5).toInt();
			pSGenReg.bDistCon2 = set.value("bDistCon2",false).toBool();
			//pSGenReg.nDistPhiL2 = set.value("nDistPhiL2",-5).toInt();
			//pSGenReg.nDistPhiH2 = set.value("nDistPhiH2",5).toInt();
			pSGenReg.nDistHorPhi = set.value("nDistHorPhi",10).toInt();
			pSGenReg.nDistAniL2 = set.value("nDistAniL2",3).toInt();
			pSGenReg.nDistAniH2 = set.value("nDistAniH2",999).toInt();
			pSGenReg.nDistInRadiusL2 = set.value("nDistInRadiusL2",0).toInt();
			pSGenReg.nDistInRadiusH2 = set.value("nDistInRadiusH2",6).toInt();

			pSGenReg.bOpening = set.value("bOpening",false).toBool();
			pSGenReg.fOpeningSize = set.value("fOpeningSize",3.5).toFloat();
			pSGenReg.bGenRoi = set.value("bGenRoi",true).toBool();
			pSGenReg.fRoiRatio = set.value("fRoiRatio",0.75).toFloat();
			pSGenReg.nClosingWH = set.value("nClosingWH",20).toInt();
			pSGenReg.nGapWH = set.value("nGapWH",100).toInt();
			pSGenReg.nMaskSize = set.value("nMaskSize",31).toInt();
			pSGenReg.nMeanGray = set.value("nMeanGray",255).toInt();
			pSGenReg.fClosingSize = set.value("fClosingSize",3.5).toFloat();
			vItemPara=QVariant::fromValue(pSGenReg);
		}
		break;
	case ITEM_SSIDEFINISH_REGION:
		{
			s_pSSideFReg pSSideFReg;
			pSSideFReg.strName = set.value("name",tr("Body Mouth Region")).toString();
			pSSideFReg.nType = set.value("type",ITEM_SSIDEFINISH_REGION).toInt();
			pSSideFReg.bEnabled = set.value("bEnabled",true).toBool();
			pSSideFReg.nShapeType = set.value("nShapeType",0).toInt();
			pSSideFReg.nEdge = set.value("nEdge",15).toInt();			
			pSSideFReg.nArea = set.value("nArea",50).toInt();
			pSSideFReg.nWidth = set.value("nWidth",5).toInt();			
			pSSideFReg.bGenRoi = set.value("bGenRoi",true).toBool();
			pSSideFReg.fRoiRatio = set.value("fRoiRatio",0.75).toFloat();
			pSSideFReg.nClosingWH = set.value("nClosingWH",20).toInt();
			pSSideFReg.nGapWH = set.value("nGapWH",50).toInt();
			pSSideFReg.nMaskSize = set.value("nMaskSize",31).toInt();
			pSSideFReg.fClosingSize = set.value("fClosingSize",3.5).toFloat();
			vItemPara=QVariant::fromValue(pSSideFReg);
		}
		break;
	case ITEM_SINFINISH_REGION:
		{
			s_pSInFReg pSInFReg;
			pSInFReg.strName = set.value("name",tr("Inner Mouth Region")).toString();
			pSInFReg.nType = set.value("type",ITEM_SINFINISH_REGION).toInt();
			pSInFReg.bEnabled = set.value("bEnabled",true).toBool();
			pSInFReg.nGray = set.value("nGray",20).toInt();
			pSInFReg.nArea = set.value("nArea",50).toInt();
			pSInFReg.fOpeningSize = set.value("fOpeningSize",1.5).toFloat();
			pSInFReg.nPos = set.value("nPos",0).toInt();
			vItemPara=QVariant::fromValue(pSInFReg);
		}
		break;
	case ITEM_SSCREWFINISH_REGION:
		{
			s_pSScrewFReg pSScrewFReg;
			pSScrewFReg.strName = set.value("name",tr("Screw Region")).toString();
			pSScrewFReg.nType = set.value("type",ITEM_SSCREWFINISH_REGION).toInt();
			pSScrewFReg.bEnabled = set.value("bEnabled",true).toBool();
			pSScrewFReg.nShapeType = set.value("nShapeType",0).toInt();
			pSScrewFReg.nEdge = set.value("nEdge",10).toInt();
			pSScrewFReg.nArea = set.value("nArea",20).toInt();
			pSScrewFReg.nLength = set.value("nLength",10).toInt();
			pSScrewFReg.nDia = set.value("nDia",1).toInt();
			pSScrewFReg.nRab = set.value("nRab",3).toInt();
			vItemPara=QVariant::fromValue(pSScrewFReg);
		}
		break;
	case ITEM_FRLINNER_REGION:
		{
			s_pFRLInReg pFRLInReg;
			pFRLInReg.strName = set.value("name",tr("Inner Ring Region")).toString();
			pFRLInReg.nType = set.value("type",ITEM_FRLINNER_REGION).toInt();
			pFRLInReg.bEnabled = set.value("bEnabled",true).toBool();
			pFRLInReg.nEdge = set.value("nEdge",15).toInt();
			pFRLInReg.nGray = set.value("nGray",50).toInt();
			pFRLInReg.nArea = set.value("nArea",35).toInt();
			pFRLInReg.nInnerRadius = set.value("nInnerRadius",3).toInt();
			pFRLInReg.fOpenSize = set.value("fOpenSize",5.5).toFloat();
			pFRLInReg.bOverPress = set.value("bOverPress",false).toBool();
			pFRLInReg.nPressEdge = set.value("nPressEdge",15).toInt();
			pFRLInReg.nPressNum = set.value("nPressNum",2).toInt();
			pFRLInReg.bInBroken = set.value("bInBroken",false).toBool();
			pFRLInReg.nBrokenEdge = set.value("nBrokenEdge",10).toInt();
			pFRLInReg.nBrokenArea = set.value("nBrokenArea",250).toInt();
			pFRLInReg.nBrokenGrayMean = set.value("nBrokenGrayMean",15).toInt();
			vItemPara=QVariant::fromValue(pFRLInReg);
		}
		break;
	case ITEM_FRLMIDDLE_REGION:
		{
			s_pFRLMidReg pFRLMidReg;
			pFRLMidReg.strName = set.value("name",tr("Middle Ring Region")).toString();
			pFRLMidReg.nType = set.value("type",ITEM_FRLMIDDLE_REGION).toInt();
			pFRLMidReg.bEnabled = set.value("bEnabled",true).toBool();
			pFRLMidReg.nEdge = set.value("nEdge",15).toInt();
			pFRLMidReg.nGray = set.value("nGray",50).toInt();
			pFRLMidReg.nArea = set.value("nArea",35).toInt();
			pFRLMidReg.nLen = set.value("nLen",15).toInt();
			pFRLMidReg.nEdge_2 = set.value("nEdge_2",30).toInt();
			pFRLMidReg.nGray_2 = set.value("nGray_2",100).toInt();
			pFRLMidReg.nArea_2 = set.value("nArea_2",70).toInt();
			pFRLMidReg.nLen_2 = set.value("nLen_2",30).toInt();
			pFRLMidReg.bOpen = set.value("bOpen",false).toBool();
			pFRLMidReg.fOpenSize = set.value("fOpenSize",3.5).toFloat();
			pFRLMidReg.bPitting = set.value("bPitting",false).toBool();
			pFRLMidReg.nPitEdge = set.value("nPitEdge",35).toInt();
			pFRLMidReg.nPitArea = set.value("nPitArea",25).toInt();
			pFRLMidReg.nPitNum = set.value("nPitNum",3).toInt();
			pFRLMidReg.bLOF = set.value("bLOF",false).toBool();
			pFRLMidReg.nLOFEdge = set.value("nLOFEdge",10).toInt();
			pFRLMidReg.fLOFRab = set.value("fLOFRab",1.5).toFloat();
			pFRLMidReg.nLOFLen = set.value("nLOFLen",70).toInt();
			pFRLMidReg.fLOFRab_In = set.value("fLOFRab_In",2.5).toFloat();
			pFRLMidReg.nLOFLen_In = set.value("nLOFLen_In",30).toInt();
			pFRLMidReg.fLOFRab_Out = set.value("fLOFRab_Out",2.5).toFloat();
			pFRLMidReg.nLOFLen_Out = set.value("nLOFLen_Out",50).toInt();
			pFRLMidReg.nLOFDiaMin = set.value("nLOFDiaMin",0).toInt();
			pFRLMidReg.nLOFDiaMax = set.value("nLOFDiaMax",10).toInt();
			pFRLMidReg.nLOFAngleOffset = set.value("nLOFAngleOffset",10).toInt();
			pFRLMidReg.bDeform = set.value("bDeform",false).toBool();
			pFRLMidReg.nDeformGary = set.value("nDeformGary",40).toInt();
			pFRLMidReg.nDeformHei = set.value("nDeformHei",4).toInt();
			pFRLMidReg.nDeformCirWid = set.value("nDeformCirWid",25).toInt();
			pFRLMidReg.bLOFOldWay = set.value("bLOFOldWay",true).toBool();
			pFRLMidReg.bLOFNewWay = set.value("bLOFNewWay",false).toBool();
			pFRLMidReg.nLOFEdge_new = set.value("nLOFEdge_new",10).toInt();
			pFRLMidReg.nLOFWidth_new = set.value("nLOFWidth_new",4).toInt();
			pFRLMidReg.nLOFLen_new = set.value("nLOFLen_new",50).toInt();
			vItemPara=QVariant::fromValue(pFRLMidReg);
		}
		break;
	case ITEM_FRLOUTER_REGION:
		{
			s_pFRLOutReg pFRLOutReg;
			pFRLOutReg.strName = set.value("name",tr("Outer Ring Region")).toString();
			pFRLOutReg.nType = set.value("type",ITEM_FRLOUTER_REGION).toInt();
			pFRLOutReg.bEnabled = set.value("bEnabled",true).toBool();
			pFRLOutReg.nEdge = set.value("nEdge",150).toInt();
			pFRLOutReg.nGray = set.value("nGray",50).toInt();
			pFRLOutReg.nArea = set.value("nArea",35).toInt();
			pFRLOutReg.fOpenSize = set.value("fOpenSize",5.5).toFloat();
			pFRLOutReg.bBreach = set.value("bBreach",false).toBool();
			pFRLOutReg.nBreachEdge = set.value("nBreachEdge",15).toInt();
			pFRLOutReg.nBreachWidth = set.value("nBreachWidth",10).toInt();
			pFRLOutReg.nBreachLen = set.value("nBreachLen",2).toInt();
			pFRLOutReg.bBrokenRing = set.value("bBrokenRing",false).toBool();
			pFRLOutReg.nBrokenRingEdge = set.value("nBrokenRingEdge",15).toInt();
			pFRLOutReg.nBrokenRingGray = set.value("nBrokenRingGray",150).toInt();
			pFRLOutReg.nBrokenRingLen = set.value("nBrokenRingLen",30).toInt();		
			pFRLOutReg.fBrokenOpenSize = set.value("fBrokenOpenSize",1.5).toFloat();		
			vItemPara=QVariant::fromValue(pFRLOutReg);
		}
		break;
	case ITEM_FBLINNER_REGION:
		{
			s_pFBLInReg pFBLInReg;
			pFBLInReg.strName = set.value("name",tr("Back Inner Region")).toString();
			pFBLInReg.nType = set.value("type",ITEM_FBLINNER_REGION).toInt();
			pFBLInReg.bEnabled = set.value("bEnabled",true).toBool();
			pFBLInReg.nEdge = set.value("nEdge",15).toInt();
			pFBLInReg.nArea = set.value("nArea",50).toInt();
			pFBLInReg.fOpenSize = set.value("fOpenSize",5.5).toFloat();
			pFBLInReg.nLOFEdge = set.value("nLOFEdge",20).toInt();
			pFBLInReg.nLOFEdgeH = set.value("nLOFEdgeH",30).toInt();
			pFBLInReg.nLOFArea = set.value("nLOFArea",10).toInt();
			pFBLInReg.nLOFHeight = set.value("nLOFHeight",5).toInt();
			pFBLInReg.fLOFOpenSize = set.value("fLOFOpenSize",7).toFloat();
			pFBLInReg.nLOFPhiL = set.value("nLOFPhiL",50).toInt();
			pFBLInReg.nLOFPhiH = set.value("nLOFPhiH",130).toInt();
			pFBLInReg.nLOFMeanGray = set.value("nLOFMeanGray",50).toInt();
			vItemPara=QVariant::fromValue(pFBLInReg);
		}
		break;
	case ITEM_FBLMIDDLE_REGION:
		{
			s_pFBLMidReg pFBLMidReg;
			pFBLMidReg.strName = set.value("name",tr("Back Middle Region")).toString();
			pFBLMidReg.nType = set.value("type",ITEM_FBLMIDDLE_REGION).toInt();
			pFBLMidReg.bEnabled = set.value("bEnabled",true).toBool();
			pFBLMidReg.nEdge = set.value("nEdge",15).toInt();
			pFBLMidReg.nArea = set.value("nArea",50).toInt();
			pFBLMidReg.bShadow = set.value("bShadow",false).toBool();
			pFBLMidReg.nShadowAng = set.value("nShadowAng",5).toInt();
			vItemPara=QVariant::fromValue(pFBLMidReg);
		}
		break;
	case ITEM_BINNER_REGION:
		{
			s_pBInReg pBInReg;
			pBInReg.strName = set.value("name",tr("Bottom Inner Region")).toString();
			pBInReg.nType = set.value("type",ITEM_BINNER_REGION).toInt();
			pBInReg.bEnabled = set.value("bEnabled",true).toBool();
			pBInReg.nMethodIdx = set.value("nMethodIdx",0).toInt();
			pBInReg.nEdge1 = set.value("nEdge1",15).toInt();
			pBInReg.nArea1 = set.value("nArea1",80).toInt();
			pBInReg.nOperation1 = set.value("nOperation1",0).toInt();
			pBInReg.nLen1 = set.value("nLen1",30).toInt();
			pBInReg.nMeanGray1 = set.value("nMeanGray1",100).toInt();
			pBInReg.nEdge2 = set.value("nEdge2",25).toInt();
			pBInReg.nArea2 = set.value("nArea2",50).toInt();
			pBInReg.nOperation2 = set.value("nOperation2",0).toInt();
			pBInReg.nLen2 = set.value("nLen2",20).toInt();
			pBInReg.nMeanGray2 = set.value("nMeanGray2",100).toInt();			
			pBInReg.bOpen = set.value("bOpen",false).toBool();
			pBInReg.fOpenSize = set.value("fOpenSize",3.5).toFloat();
			pBInReg.bBottomDL = set.value("bBottomDL",false).toBool();
			pBInReg.nDarkB = set.value("nDarkB",10).toInt();
			pBInReg.nLightB = set.value("nLightB",240).toInt();
			pBInReg.bArc = set.value("bArc",false).toBool();
			pBInReg.nArcEdge = set.value("nArcEdge",10).toInt();
			pBInReg.nArcWidth = set.value("nArcWidth",5).toInt();
			pBInReg.bChar = set.value("bChar",false).toBool();
			pBInReg.nCharNum = set.value("nCharNum",2).toInt();
			pBInReg.fNeiRadius = set.value("fNeiRadius",25.5).toFloat();
			pBInReg.dModelPhi = set.value("dModelPhi",0).toFloat();
			pBInReg.bDoubleScale = set.value("bDoubleScale",false).toBool();
			pBInReg.nEdgeMin = set.value("nEdgeMin",15).toInt();
			pBInReg.nEdgeMax = set.value("nEdgeMax",25).toInt();
			pBInReg.nAreaMin = set.value("nAreaMin",35).toInt();
			pBInReg.fScaleRatio = set.value("fScaleRatio",0.7).toFloat();
			pBInReg.bSpot = set.value("bSpot",false).toBool();
			pBInReg.nAreaSpot = set.value("nAreaSpot",25).toInt();
			pBInReg.fSpotRound = set.value("fSpotRound",0.8).toFloat();
			pBInReg.nSpotNum = set.value("nSpotNum",1).toInt();

			pBInReg.bStrip = set.value("bStrip",false).toBool();
			pBInReg.nStripEdge = set.value("nStripEdge",5).toInt();
			pBInReg.nStripArea = set.value("nStripArea",30).toInt();
			pBInReg.nStripHeight = set.value("nStripHeight",50).toInt();
			pBInReg.nStripWidthL = set.value("nStripWidthL",0).toInt();
			pBInReg.nStripWidthH = set.value("nStripWidthH",999).toInt();
			pBInReg.fStripRab = set.value("fStripRab",3.5).toFloat();
			pBInReg.nStripAngleL = set.value("nStripAngleL",70).toInt();
			pBInReg.nStripAngleH = set.value("nStripAngleH",110).toInt();
			vItemPara=QVariant::fromValue(pBInReg);
		}
		break;
	case ITEM_BMIDDLE_REGION:
		{
			s_pBMidReg pBMidReg;
			pBMidReg.strName = set.value("name",tr("Bottom Middle Region")).toString();
			pBMidReg.nType = set.value("type",ITEM_BMIDDLE_REGION).toInt();
			pBMidReg.bEnabled = set.value("bEnabled",true).toBool();
			pBMidReg.nEdge1 = set.value("nEdge1",15).toInt();
			pBMidReg.nArea1 = set.value("nArea1",80).toInt();
			pBMidReg.nOperation1 = set.value("nOperation1",0).toInt();
			pBMidReg.nLen1 = set.value("nLen1",30).toInt();
			pBMidReg.nMeanGray1 = set.value("nMeanGray1",100).toInt();			
			pBMidReg.nEdge2 = set.value("nEdge2",25).toInt();
			pBMidReg.nArea2 = set.value("nArea2",50).toInt();
			pBMidReg.nOperation2 = set.value("nOperation2",0).toInt();
			pBMidReg.nLen2 = set.value("nLen2",20).toInt();
			pBMidReg.nMeanGray2 = set.value("nMeanGray2",100).toInt();			
			pBMidReg.bOpen = set.value("bOpen",false).toBool();
			pBMidReg.fOpenSize = set.value("fOpenSize",3.5).toFloat();
			pBMidReg.bArc = set.value("bArc",false).toBool();
			pBMidReg.nArcEdge = set.value("nArcEdge",10).toInt();
			pBMidReg.nArcWidth = set.value("nArcWidth",5).toInt();
			vItemPara=QVariant::fromValue(pBMidReg);
		}
		break;
	case ITEM_BOUTER_REGION:
		{
			s_pBOutReg pBOutReg;
			pBOutReg.strName = set.value("name",tr("Bottom Outer Region")).toString();
			pBOutReg.nType = set.value("type",ITEM_BOUTER_REGION).toInt();
			pBOutReg.bEnabled = set.value("bEnabled",true).toBool();
			pBOutReg.nEdge = set.value("nEdge",15).toInt();
			pBOutReg.nGray = set.value("nGray",50).toInt();
			pBOutReg.nArea = set.value("nArea",80).toInt();
			pBOutReg.nOperation = set.value("nOperation",0).toInt();
			pBOutReg.nLen = set.value("nLen",30).toInt();
			pBOutReg.nLenMax = set.value("nLenMax",9999).toInt();
			pBOutReg.fOpenSize = set.value("fOpenSize",9.5).toFloat();
			//检灌装线瓶底外环条纹
			pBOutReg.bStrip = set.value("bStrip",false).toBool();
			pBOutReg.nStripEdge = set.value("nStripEdge",5).toInt();
			pBOutReg.nStripArea = set.value("nStripArea",30).toInt();
			pBOutReg.nStripHeight = set.value("nStripHeight",50).toInt();
			pBOutReg.nStripWidthL = set.value("nStripWidthL",0).toInt();
			pBOutReg.nStripWidthH = set.value("nStripWidthH",999).toInt();
			pBOutReg.fStripRab = set.value("fStripRab",3.5).toFloat();
			pBOutReg.nStripAngleL = set.value("nStripAngleL",70).toInt();
			pBOutReg.nStripAngleH = set.value("nStripAngleH",110).toInt();	
			vItemPara=QVariant::fromValue(pBOutReg);
		}
		break;
	case ITEM_SSIDEWALL_REGION:
		{
			s_pSSideReg pSSideReg;
			pSSideReg.strName = set.value("name",tr("Body Stress Region")).toString();
			pSSideReg.nType = set.value("type",ITEM_SSIDEWALL_REGION).toInt();
			pSSideReg.bEnabled = set.value("bEnabled",true).toBool();
			pSSideReg.nShapeType = set.value("nShapeType",0).toInt();
			pSSideReg.nEdge = set.value("nEdge",15).toInt();
			pSSideReg.nGray = set.value("nGray",100).toInt();
			pSSideReg.nArea = set.value("nArea",80).toInt();
			pSSideReg.fRab = set.value("fRab",5.0).toFloat();
			pSSideReg.bInspItem = set.value("bInspItem",0).toInt();
			pSSideReg.nSideDistance = set.value("nSideDistance",2).toInt();

			pSSideReg.bDistCon1 = set.value("bDistCon1",false).toBool();
			pSSideReg.nDistVerPhi = set.value("nDistVerPhi",10).toInt();
			pSSideReg.nDistAniL1 = set.value("nDistAniL1",3).toInt();
			pSSideReg.nDistAniH1 = set.value("nDistAniH1",999).toInt();
			pSSideReg.nDistInRadiusL1 = set.value("nDistInRadiusL1",0).toInt();
			pSSideReg.nDistInRadiusH1 = set.value("nDistInRadiusH1",5).toInt();
			pSSideReg.bDistCon2 = set.value("bDistCon2",false).toBool();
			pSSideReg.nDistHorPhi = set.value("nDistHorPhi",10).toInt();
			pSSideReg.nDistAniL2 = set.value("nDistAniL2",3).toInt();
			pSSideReg.nDistAniH2 = set.value("nDistAniH2",999).toInt();
			pSSideReg.nDistInRadiusL2 = set.value("nDistInRadiusL2",0).toInt();
			pSSideReg.nDistInRadiusH2 = set.value("nDistInRadiusH2",6).toInt();
			vItemPara=QVariant::fromValue(pSSideReg);
			//printf("%s--%s--%s--%s###","pSSideReg.strName:", pSSideReg.strName,pSSideReg.strName.toStdString().data(), QString::fromLocal8Bit(pSSideReg.strName));

		}
		break;
	case ITEM_DISTURB_REGION:
		{
			s_pDistReg pDistReg;
			pDistReg.strName = set.value("name",tr("Disturb Region")).toString();
			pDistReg.nType = set.value("type",ITEM_DISTURB_REGION).toInt();
			pDistReg.bEnabled = set.value("bEnabled",true).toBool();
			pDistReg.nRegType = set.value("nRegType",0).toInt();
			pDistReg.nShapeType = set.value("nShapeType",0).toInt();
			pDistReg.bStripeSelf = set.value("bStripeSelf",false).toBool();
			pDistReg.fOpenSize = set.value("fOpenSize",3.5).toFloat();
			pDistReg.bIsMove = set.value("bIsMove",true).toBool();
			pDistReg.nStripeEdge = set.value("nStripeEdge",10).toInt();
			pDistReg.nStripeMaskSize = set.value("nStripeMaskSize",31).toInt();
			pDistReg.bVertStripe = set.value("bVertStripe",false).toBool();
			pDistReg.nVertAng = set.value("nVertAng",10).toInt();
			pDistReg.nVertWidthL = set.value("nVertWidthL",0).toInt();
			pDistReg.nVertWidthH = set.value("nVertWidthH",10).toInt();
			pDistReg.fVertRabL = set.value("fVertRabL",3).toFloat();
			pDistReg.fVertRabH = set.value("fVertRabH",999).toFloat();
			pDistReg.fVertInRadiusL = set.value("fVertInRadiusL",0).toFloat();
			pDistReg.fVertInRadiusH = set.value("fVertInRadiusH",999).toFloat();
			pDistReg.bHoriStripe = set.value("bHoriStripe",false).toBool();
			pDistReg.nHoriAng = set.value("nHoriAng",10).toInt();
			pDistReg.nHoriWidthL = set.value("nHoriWidthL",0).toInt();
			pDistReg.nHoriWidthH = set.value("nHoriWidthH",10).toInt();
			pDistReg.fHoriRabL = set.value("fHoriRabL",3).toFloat();
			pDistReg.fHoriRabH = set.value("fHoriRabH",999).toFloat();
			pDistReg.fHoriInRadiusL = set.value("fHoriInRadiusL",0).toFloat();
			pDistReg.fHoriInRadiusH = set.value("fHoriInRadiusH",999).toFloat();
			pDistReg.fBubbleCir = set.value("fBubbleCir",0.5).toFloat();
			pDistReg.nBubbleArea = set.value("nBubbleArea",50).toInt();
			pDistReg.nBubbleLowThre = set.value("nBubbleLowThre",10).toInt();
			pDistReg.nBubbleHighThre = set.value("nBubbleHighThre",20).toInt();
			vItemPara=QVariant::fromValue(pDistReg);
		}
		break;
	case ITEM_FINISH_CONTOUR:
		{
			s_pFinCont pFinCont;
			pFinCont.strName = set.value("name",tr("Mouth Contour")).toString();
			pFinCont.nType = set.value("type",ITEM_FINISH_CONTOUR).toInt();
			pFinCont.bEnabled = set.value("bEnabled",true).toBool();
			pFinCont.nSubThresh = set.value("nSubThresh",25).toInt();
			pFinCont.nRegWidth = set.value("nRegWidth",200).toInt();
			pFinCont.nRegHeight = set.value("nRegHeight",50).toInt();
			pFinCont.nGapWidth = set.value("nGapWidth",4).toInt();
			pFinCont.nGapHeight = set.value("nGapHeight",3).toInt();
			pFinCont.nNotchHeight = set.value("nNotchHeight",3).toInt();
			pFinCont.nArea = set.value("nArea",20).toInt();
			pFinCont.bCheckOverPress = set.value("bCheckOverPress",false).toBool();
			vItemPara=QVariant::fromValue(pFinCont);
		}
		break;
	case ITEM_NECK_CONTOUR:
		{
			s_pNeckCont pNeckCont;
			pNeckCont.strName = set.value("name",tr("Shoulder Contour")).toString();
			pNeckCont.nType = set.value("type",ITEM_NECK_CONTOUR).toInt();
			pNeckCont.bEnabled = set.value("bEnabled",true).toBool();
			pNeckCont.nThresh = set.value("nThresh",10).toInt();
			pNeckCont.nNeckHeiL = set.value("nNeckHeiL",1).toInt();
			pNeckCont.nNeckHeiH = set.value("nNeckHeiH",999).toInt();
			pNeckCont.nArea = set.value("nArea",100).toInt();
			vItemPara=QVariant::fromValue(pNeckCont);
		}
		break;
	case ITEM_BODY_CONTOUR:
		{
			s_pBodyCont pBodyCont;
			pBodyCont.strName = set.value("name",tr("Body Contour")).toString();
			pBodyCont.nType = set.value("type",ITEM_BODY_CONTOUR).toInt();
			pBodyCont.bEnabled = set.value("bEnabled",true).toBool();
			pBodyCont.nThresh = set.value("nThresh",10).toInt();
			pBodyCont.nWidth = set.value("nWidth",10).toInt();
			pBodyCont.nArea = set.value("nArea",100).toInt();
			vItemPara=QVariant::fromValue(pBodyCont);
		}
		break;
	case ITEM_SBRISPOT_REGION:
		{
			s_pSBriSpotReg pSBriSpotReg;
			pSBriSpotReg.strName = set.value("name",tr("Bright Spots")).toString();
			pSBriSpotReg.nType = set.value("type",ITEM_SBRISPOT_REGION).toInt();
			pSBriSpotReg.bEnabled = set.value("bEnabled",true).toBool();
			pSBriSpotReg.nShapeType = set.value("nShapeType",0).toInt();
			pSBriSpotReg.nGray = set.value("nGray",230).toInt();
			pSBriSpotReg.fOpenRadius = set.value("fOpenRadius",5.5).toFloat();
			pSBriSpotReg.nArea = set.value("nArea",300).toInt();
			vItemPara=QVariant::fromValue(pSBriSpotReg);
		}
		break;
	case ITEM_BALL_REGION:
		{
			s_pBAllReg pBAllReg;
			pBAllReg.strName = set.value("name",tr("Bottom All Region")).toString();
			pBAllReg.nType = set.value("type",ITEM_BALL_REGION).toInt();
			pBAllReg.bEnabled = set.value("bEnabled",true).toBool();
			pBAllReg.bModeNO = set.value("bModeNO",false).toBool();
			pBAllReg.nModePointEdge = set.value("nModePointEdge",35).toInt();
			pBAllReg.nModeNOWidth = set.value("nModeNOWidth",200).toInt();
			pBAllReg.nModeNOHeight = set.value("nModeNOHeight",20).toInt();
			pBAllReg.nModePointNum = set.value("nModePointNum",9).toInt();
			pBAllReg.fModePointRadius = set.value("fModePointRadius",3.5).toFloat();
			pBAllReg.nMaxModePointSpace = set.value("nMaxModePointSpace",80).toInt();

			pBAllReg.nMaskWidth = set.value("nMaskWidth",21).toInt();
			pBAllReg.nMaskHeight = set.value("nMaskHeight",21).toInt();
			pBAllReg.nFontCloseScale = set.value("nFontCloseScale",8).toInt();
			pBAllReg.nModePointSobel = set.value("nModePointSobel",20).toInt();
			pBAllReg.nMinModePointHeight = set.value("nMinModePointHeight",6).toInt();
			pBAllReg.fSobelOpeningScale = set.value("fSobelOpeningScale",6.5).toFloat();

			pBAllReg.bModeNOExt = set.value("bModeNOExt",true).toBool();
			pBAllReg.nMinModePointNumExt = set.value("nMinModePointNumExt",6).toInt();
			pBAllReg.nSigPointDisExt = set.value("nSigPointDisExt",40).toInt();
			pBAllReg.bDelStripe = set.value("bDelStripe",false).toBool();
			pBAllReg.nStrLengthth = set.value("nStrLengthth",450).toInt();
			pBAllReg.nStrWidth = set.value("nStrWidth",200).toInt();
			pBAllReg.bFlagThirtMp = set.value("Checkthirtmp",false).toBool();
			pBAllReg.nMouldEdge = set.value("nMouldEdge",20).toInt();
			pBAllReg.nMouldDia = set.value("nMouldDia",25).toInt();
			pBAllReg.nMouldSpace = set.value("nMouldSpace",10).toInt();
			pBAllReg.bCheckAntiClockwise = set.value("bCheckAntiClockwise",true).toBool();
            pBAllReg.nMouldInnerDiaL = set.value("nMouldInnerDiaL",2).toInt();
			pBAllReg.nMouldInnerDiaH = set.value("nMouldInnerDiaH",5).toInt();
			pBAllReg.bCheckIfReportError = set.value("bCheckIfReportError",false).toBool();
			pBAllReg.nModeReject_1 = set.value("nModeReject_1",0).toInt();
			pBAllReg.nModeReject_2 = set.value("nModeReject_2",0).toInt();
			pBAllReg.nModeReject_3 = set.value("nModeReject_3",0).toInt();
			pBAllReg.strModeReject = set.value("strModeReject",tr("")).toString();
			vItemPara=QVariant::fromValue(pBAllReg);
		}
		break;
	case ITEM_CIRCLE_SIZE:
		{
			s_pCirSize pCirSize;
			pCirSize.strName = set.value("name",tr("Circle Size")).toString();
			pCirSize.nType = set.value("type",ITEM_CIRCLE_SIZE).toInt();
			pCirSize.bEnabled = set.value("bEnabled",true).toBool();
			pCirSize.nEdge = set.value("nEdge",15).toInt();		
			pCirSize.bDia = set.value("bDia",false).toBool();
			pCirSize.fDiaCurValue = set.value("fDiaCurValue",0).toFloat();
			pCirSize.fDiaLower = set.value("fDiaLower",0).toFloat();
			pCirSize.fDiaUpper = set.value("fDiaUpper",999.9).toFloat();
			pCirSize.fDiaReal = set.value("fDiaReal",0).toFloat();
			pCirSize.fDiaModify = set.value("fDiaModify",0.3).toFloat();
			pCirSize.fDiaRuler = set.value("fDiaRuler",1).toFloat();
			pCirSize.bOvality = set.value("bOvality",false).toBool();
			pCirSize.fOvalCurValue = set.value("fOvalCurValue",0).toFloat();
			pCirSize.fOvality = set.value("fOvality",10).toFloat();
			vItemPara=QVariant::fromValue(pCirSize);
		}
		break;
	case ITEM_SBASE_REGION:
		{
			s_pSBaseReg pSBaseReg;
			pSBaseReg.strName = set.value("name",tr("Base Stress Region")).toString();
			pSBaseReg.nType = set.value("type",ITEM_SBASE_REGION).toInt();
			pSBaseReg.bEnabled = set.value("bEnabled",true).toBool();
			pSBaseReg.nMethodIdx = set.value("nMethodIdx",0).toInt();
			pSBaseReg.nEdge = set.value("nEdge",15).toInt();
			pSBaseReg.nGray = set.value("nGray",100).toInt();
			pSBaseReg.nArea = set.value("nArea",80).toInt();
			pSBaseReg.fRab = set.value("fRab",5.0).toFloat();
			pSBaseReg.nSideDistance = set.value("nSideDistance",2).toInt();
			vItemPara=QVariant::fromValue(pSBaseReg);
		}
		break;
	case ITEM_SBASECONVEX_REGION:
		{
			s_pSBaseConvexReg pSBaseConvexReg;
			pSBaseConvexReg.strName = set.value("name",tr("Base Convex Region")).toString();
			pSBaseConvexReg.nType = set.value("type",ITEM_SBASECONVEX_REGION).toInt();
			pSBaseConvexReg.bEnabled = set.value("bEnabled",true).toBool();
			pSBaseConvexReg.bGenDefects = set.value("bGenDefects",false).toBool();
			pSBaseConvexReg.nEdge = set.value("nEdge",15).toInt();
			pSBaseConvexReg.nGray = set.value("nGray",30).toInt();
			pSBaseConvexReg.nArea = set.value("nArea",100).toInt();
			pSBaseConvexReg.nLength = set.value("nLength",30).toInt();
			pSBaseConvexReg.bContDefects = set.value("bContDefects",false).toBool();
			pSBaseConvexReg.nContGray = set.value("nContGray",10).toInt();
			pSBaseConvexReg.nContArea = set.value("nContArea",50).toInt();
			pSBaseConvexReg.nContLength = set.value("nContLength",30).toInt();
			pSBaseConvexReg.fOpeningSize = set.value("fOpeningSize",4.5).toFloat();
			
			pSBaseConvexReg.bGenRoi = set.value("bGenRoi",true).toBool();
			pSBaseConvexReg.fRoiRatio = set.value("fRoiRatio",0.75).toFloat();
			pSBaseConvexReg.nClosingWH = set.value("nClosingWH",20).toInt();
			pSBaseConvexReg.nGapWH = set.value("nGapWH",100).toInt();
			pSBaseConvexReg.nMaskSize = set.value("nMaskSize",31).toInt();
			pSBaseConvexReg.fClosingSize = set.value("fClosingSize",3.5).toFloat();
			vItemPara=QVariant::fromValue(pSBaseConvexReg);
		}
		break;
	default:
		break;
	}

	return vItemPara;
}

//*功能：根据类型，读入形状数据
QVariant CCheck::readShapebyType(int nType,QSettings &set)
{
	QVariant vItemShape;
	switch(nType)
	{
	case ITEM_SIDEWALL_LOCATE:
		{
			s_oSideLoc oSideLoc;
			int x0,y0,x1,y1;
			set.beginGroup("offset");
			oSideLoc.drow1 = set.value("drow1",0).toDouble();
			oSideLoc.dcol1 = set.value("dcol1",0).toDouble();
			oSideLoc.dphi1 = set.value("dphi1",0).toDouble();
			oSideLoc.drow2 = set.value("drow2",0).toDouble();
			oSideLoc.dcol2 = set.value("dcol2",0).toDouble();
			oSideLoc.dphi2 = set.value("dphi2",0).toDouble();
			oSideLoc.nLeftRight = set.value("nLeftRight",1).toInt();
			set.endGroup();
			set.beginGroup("origin");
			oSideLoc.ori.Row = set.value("Row",0).toFloat();
			oSideLoc.ori.Col = set.value("Col",0).toFloat();
			oSideLoc.ori.Angle = set.value("Angle",0).toFloat();
			oSideLoc.ori.nRow11 = set.value("nRow11",0).toInt();
			oSideLoc.ori.nCol11 = set.value("nCol11",0).toInt();
			oSideLoc.ori.nRow12 = set.value("nRow12",0).toInt();
			oSideLoc.ori.nCol12 = set.value("nCol12",0).toInt();
			oSideLoc.ori.nRow21 = set.value("nRow21",0).toInt();
			oSideLoc.ori.nCol21 = set.value("nCol21",0).toInt();
			oSideLoc.ori.nRow22 = set.value("nRow22",0).toInt();
			oSideLoc.ori.nCol22 = set.value("nCol22",0).toInt();
			oSideLoc.ori.fDist1 = set.value("fDist1",0).toFloat();
			oSideLoc.ori.fDist2 = set.value("fDist2",0).toFloat();
			oSideLoc.ori.fDist3 = set.value("fDist3",0).toFloat();
			set.endGroup();
			set.beginGroup("firstLine");
			x0 = set.value("x0",20).toInt();
			y0 = set.value("y0",30).toInt();
			x1 = set.value("x1",200).toInt();
			y1 = set.value("y1",30).toInt();
			gen_region_line(&oSideLoc.oFirstLine,y0,x0,y1,x1);
			set.endGroup();
			set.beginGroup("secondLine");
			x0 = set.value("x0",20).toInt();
			y0 = set.value("y0",30).toInt();
			x1 = set.value("x1",200).toInt();
			y1 = set.value("y1",30).toInt();
			gen_region_line(&oSideLoc.oSecondLine,y0,x0,y1,x1);
			set.endGroup();
			set.beginGroup("thirdLine");
			x0 = set.value("x0",20).toInt();
			y0 = set.value("y0",30).toInt();
			x1 = set.value("x1",200).toInt();
			y1 = set.value("y1",30).toInt();
			gen_region_line(&oSideLoc.oThirdLine,y0,x0,y1,x1);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSideLoc);
		}
		break;
	case  ITEM_FINISH_LOCATE:
		{
			s_oFinLoc oFinLoc;
			float fRow,fCol,fRadius;
			set.beginGroup("origin");
			oFinLoc.Row = set.value("Row",0).toFloat();
			oFinLoc.Col = set.value("Col",0).toFloat();
			oFinLoc.Radius = set.value("Radius",10).toFloat();
			set.endGroup();
			set.beginGroup("inCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFinLoc.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("outCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFinLoc.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oFinLoc);
		}
		break;
	case ITEM_BASE_LOCATE:
		{
			s_oBaseLoc oBaseLoc;
			float fRow,fCol,fRadius;
			float fAngle;
			set.beginGroup("offset");
			oBaseLoc.drow1 = set.value("drow1",0).toDouble();
			oBaseLoc.dcol1 = set.value("dcol1",0).toDouble();
			oBaseLoc.dradius1 = set.value("dradius1",0).toDouble();
			oBaseLoc.drow2 = set.value("drow2",0).toDouble();
			oBaseLoc.dcol2 = set.value("dcol2",0).toDouble();
			oBaseLoc.dradius2 = set.value("dradius2",0).toDouble();
			set.endGroup();
			set.beginGroup("origin");
			oBaseLoc.Row = set.value("Row",0).toFloat();
			oBaseLoc.Col = set.value("Col",0).toFloat();
			oBaseLoc.Radius = set.value("Radius",10).toFloat();
			oBaseLoc.Angle = set.value("Angle", 0).toFloat();
			set.endGroup();
			set.beginGroup("centReg");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBaseLoc.oCentReg,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oModeNOEdge");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBaseLoc.oModeNOEdge,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("beltDia");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBaseLoc.oBeltDia,fRow,fCol,fRadius);
			set.endGroup();
			oBaseLoc.ifGenSp = false; //init false
			double row1,row2,col1,col2;
		/*	set.beginGroup("oModeNOEdge_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oBaseLoc.oModeNOEdge_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
*/
			set.beginGroup("beltDia_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			//gen_rectangle2(&oBaseLoc.oBeltDia_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			gen_rectangle2_contour_xld(&oBaseLoc.oBeltDia_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();

			/*set.beginGroup("oCentReg_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oBaseLoc.oCentReg_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);*/
			set.endGroup();
			vItemShape = QVariant::fromValue(oBaseLoc);
		}
		break;
	case ITEM_HORI_SIZE:
		{
			s_oHoriSize oHoriSize;
			double row1,row2,col1,col2;
			set.beginGroup("ptLeft");
			oHoriSize.ptLeft.setX(set.value("x",0).toInt());
			oHoriSize.ptLeft.setY(set.value("y",0).toInt());
			set.endGroup();
			set.beginGroup("ptRight");
			oHoriSize.ptRight.setX(set.value("x",0).toInt());
			oHoriSize.ptRight.setY(set.value("y",0).toInt());
			set.endGroup();
			set.beginGroup("sizeRect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			//gen_rectangle1(&oHoriSize.oSizeRect,row1,col1,row2,col2);
			gen_rectangle2_contour_xld(&oHoriSize.oSizeRect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oHoriSize);
		}
		break;
	case ITEM_VERT_SIZE:
		{
			s_oVertSize oVertSize;
			double row1,row2,col1,col2;
			set.beginGroup("ptLeft");
			oVertSize.ptLeft.setX(set.value("x",0).toInt());
			oVertSize.ptLeft.setY(set.value("y",0).toInt());
			set.endGroup();
			set.beginGroup("ptRight");
			oVertSize.ptRight.setX(set.value("x",0).toInt());
			oVertSize.ptRight.setY(set.value("y",0).toInt());
			set.endGroup();
			set.beginGroup("sizeRect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			//gen_rectangle1(&oVertSize.oSizeRect,row1,col1,row2,col2);
			gen_rectangle2_contour_xld(&oVertSize.oSizeRect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oVertSize);
		}
		break;
	case ITEM_FULL_HEIGHT:
		{
			s_oFullHeight oFullHeight;
			double row1,row2,col1,col2;
			set.beginGroup("ptLeft");
			oFullHeight.ptLeft.setX(set.value("x",0).toInt());
			oFullHeight.ptLeft.setY(set.value("y",0).toInt());
			set.endGroup();
			set.beginGroup("ptRight");
			oFullHeight.ptRight.setX(set.value("x",0).toInt());
			oFullHeight.ptRight.setY(set.value("y",0).toInt());
			set.endGroup();
			set.beginGroup("sizeRect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			//gen_rectangle1(&oVertSize.oSizeRect,row1,col1,row2,col2);
			gen_rectangle2_contour_xld(&oFullHeight.oSizeRect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oFullHeight);
		}
		break;
	case ITEM_BENT_NECK:
		{
			s_oBentNeck oBentNeck;
			double row1,row2,col1,col2;
			set.beginGroup("finRect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			//gen_rectangle1(&oBentNeck.oFinRect,row1,col1,row2,col2);
			gen_rectangle2_contour_xld(&oBentNeck.oFinRect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oBentNeck);
		}
		break;
	case ITEM_VERT_ANG:
		{
			s_oVertAng oVertAng;					
			vItemShape = QVariant::fromValue(oVertAng);
		}
		break;
	case ITEM_SGENNERAL_REGION:
		{
			s_oSGenReg oSGenReg;

			// 2017.2---默认多边形
			set.beginGroup("checkRegion");
			readXLDbyPts(set,&oSGenReg.oCheckRegion);
			set.endGroup();
			// 2017.2---新增矩形
			double row1,row2,col1,col2;
			set.beginGroup("checkRegion_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oSGenReg.oCheckRegion_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();

			vItemShape = QVariant::fromValue(oSGenReg);
		}
		break;
	case ITEM_DISTURB_REGION:
		{
			s_oDistReg oDistReg;			
			set.beginGroup("disturbReg");
			readXLDbyPts(set,&oDistReg.oDisturbReg);
			set.endGroup();
			// 2017.11---新增矩形
			double row1,row2,col1,col2;
			set.beginGroup("disturbReg_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oDistReg.oDisturbReg_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oDistReg);
		}
		break;
	case ITEM_SSIDEFINISH_REGION:
		{
			s_oSSideFReg oSSideFReg;			
			set.beginGroup("checkRegion");
			readXLDbyPts(set,&oSSideFReg.oCheckRegion);
			set.endGroup();
			// 2017.11---新增矩形
			double row1,row2,col1,col2;
			set.beginGroup("checkRegion_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oSSideFReg.oCheckRegion_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSSideFReg);
		}
		break;
	case ITEM_SINFINISH_REGION:
		{
			s_oSInFReg oSInFReg;			
			set.beginGroup("checkRegion");
			readXLDbyPts(set,&oSInFReg.oCheckRegion);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSInFReg);
		}
		break;
	case ITEM_SSCREWFINISH_REGION:
		{
			s_oSScrewFReg oSScrewFReg;			
			set.beginGroup("checkRegion");
			readXLDbyPts(set,&oSScrewFReg.oCheckRegion);
			set.endGroup();
			// 2017.11---新增矩形
			double row1,row2,col1,col2;
			set.beginGroup("checkRegion_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oSScrewFReg.oCheckRegion_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSScrewFReg);
		}
		break;
	case ITEM_FRLINNER_REGION:
		{
			s_oFRLInReg oFRLInReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFRLInReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFRLInReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oFRLInReg);
		}
		break;
	case ITEM_FRLMIDDLE_REGION:
		{
			s_oFRLMidReg oFRLMidReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFRLMidReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFRLMidReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oFRLMidReg);
		}
		break;
	case ITEM_FRLOUTER_REGION:
		{
			s_oFRLOutReg oFRLOutReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFRLOutReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFRLOutReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oFRLOutReg);
		}
		break;
	case ITEM_FBLINNER_REGION:
		{
			s_oFBLInReg oFBLInReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFBLInReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFBLInReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();

			set.beginGroup("oPolygon");
			readXLDbyPts(set,&oFBLInReg.oPolygon);
			set.endGroup();

			vItemShape = QVariant::fromValue(oFBLInReg);
		}
		break;
	case ITEM_FBLMIDDLE_REGION:
		{
			s_oFBLMidReg oFBLMidReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFBLMidReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oFBLMidReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oFBLMidReg);
		}
		break;
	case ITEM_BINNER_REGION:
		{
			s_oBInReg oBInReg;
			set.beginGroup("oTriBase");
			readXLDbyPts(set,&oBInReg.oTriBase);
			set.endGroup();

			set.beginGroup("oRectBase");
			readXLDbyPts(set,&oBInReg.oRectBase);
			set.endGroup();

			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBInReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBInReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oMarkReg");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBInReg.oMarkReg,fRow,fCol,fRadius);
			gen_circle(&oBInReg.oCurMarkReg,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oCharReg");
			readXLDbyPts(set,&oBInReg.oCharReg);
			readXLDbyPts(set,&oBInReg.oCurCharReg);
			set.endGroup();
			oBInReg.ModelID = m_nBaseCharModelID;
			vItemShape = QVariant::fromValue(oBInReg);
		}
		break;
	case ITEM_BMIDDLE_REGION:
		{
			s_oBMidReg oBMidReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBMidReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBMidReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oBMidReg);
		}
		break;
	case ITEM_BOUTER_REGION:
		{
			s_oBOutReg oBOutReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBOutReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBOutReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oBOutReg);
		}
		break;	
	case ITEM_SSIDEWALL_REGION:
		{
			s_oSSideReg oSSideReg;			
			set.beginGroup("checkRegion");
			readXLDbyPts(set,&oSSideReg.oCheckRegion);
			set.endGroup();
			// 2017.11---新增矩形
			double row1,row2,col1,col2;
			set.beginGroup("checkRegion_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oSSideReg.oCheckRegion_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSSideReg);
		}
		break;
	case ITEM_FINISH_CONTOUR:
		{
			s_oFinCont oFinCont;
			vItemShape = QVariant::fromValue(oFinCont);
		}
		break;
	case ITEM_NECK_CONTOUR:
		{
			s_oNeckCont oNeckCont;
			double row1,row2,col1,col2;			
			set.beginGroup("checkRegion");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			//gen_rectangle1(&oNeckCont.oCheckRegion,row1,col1,row2,col2);
			gen_rectangle2_contour_xld(&oNeckCont.oCheckRegion,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oNeckCont);
		}
		break;
	case ITEM_BODY_CONTOUR:
		{
			s_oBodyCont oBodyCont;
			double row1,row2,col1,col2;			
			set.beginGroup("checkRegion");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			//gen_rectangle1(&oBodyCont.oCheckRegion,row1,col1,row2,col2);
			gen_rectangle2_contour_xld(&oBodyCont.oCheckRegion,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oBodyCont);
		}
		break;
	case ITEM_SBRISPOT_REGION:
		{
			s_oSBriSpotReg oSBriSpotReg;
			set.beginGroup("checkRegion");
			readXLDbyPts(set,&oSBriSpotReg.oCheckRegion);
			set.endGroup();
			// 2017.11---新增矩形
			double row1,row2,col1,col2;
			set.beginGroup("checkRegion_Rect");
			row1 = set.value("row1",10).toDouble();
			col1 = set.value("col1",10).toDouble();
			row2 = set.value("row2",100).toDouble();
			col2 = set.value("col2",100).toDouble();
			gen_rectangle2_contour_xld(&oSBriSpotReg.oCheckRegion_Rect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,(row2-row1)/2);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSBriSpotReg);
		}
		break;
	case ITEM_BALL_REGION:
		{
			s_oBAllReg oBAllReg;
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBAllReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oBAllReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oBAllReg);
		}
		break;
	case ITEM_CIRCLE_SIZE:
		{
			s_oCirSize oCirSize;
			double fRow,fCol,fRadius;
			set.beginGroup("oCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oCirSize.oCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oCirSize);		
		}
		break;
	case ITEM_SBASE_REGION:
		{
			s_oSBaseReg oSBaseReg;			
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oSBaseReg.oInCircle,fRow,fCol,fRadius);
			set.endGroup();
			set.beginGroup("oOutCircle");
			fRow = set.value("fRow",100).toFloat();
			fCol = set.value("fCol",100).toFloat();
			fRadius = set.value("fRadius",50).toFloat();
			gen_circle(&oSBaseReg.oOutCircle,fRow,fCol,fRadius);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSBaseReg);
		}
		break;
	case ITEM_SBASECONVEX_REGION:
		{
			s_oSBaseConvexReg oSBaseConvexReg;			
			set.beginGroup("checkRegion");
			readXLDbyPts(set,&oSBaseConvexReg.oCheckRegion);
			set.endGroup();
			vItemShape = QVariant::fromValue(oSBaseConvexReg);
		}
		break;
	default:
		break;
	}	
	return vItemShape;
}

//*功能：根据类型，保存参数数据
void CCheck::saveParabyType(int nType,QSettings &set,QVariant &vItemPara,bool bCopy/*=false*/)
{
	//注意：浮点型需要转化为字符串存储，否则乱码
	switch(nType)
	{
	case ITEM_SIDEWALL_LOCATE:
		{
			s_pSideLoc pSideLoc = vItemPara.value<s_pSideLoc>();
			set.setValue("name",pSideLoc.strName);
			set.setValue("type",pSideLoc.nType);
			set.setValue("bEnabled",pSideLoc.bEnabled);
			set.setValue("nMethodIdx",pSideLoc.nMethodIdx);
			set.setValue("bStress",pSideLoc.bStress);
			set.setValue("nFloatRange",pSideLoc.nFloatRange);
			//set.setValue("nLeftRight",pSideLoc.nLeftRight);
			set.setValue("nDirect",pSideLoc.nDirect);
			set.setValue("nEdge",pSideLoc.nEdge);
			set.setValue("bFindPointSubPix",pSideLoc.bFindPointSubPix);
		}
		break;
	case ITEM_FINISH_LOCATE:
		{			
			s_pFinLoc pFinLoc = vItemPara.value<s_pFinLoc>();	
			set.setValue("name",pFinLoc.strName);
			set.setValue("type",pFinLoc.nType);
			set.setValue("bEnabled",pFinLoc.bEnabled);
			set.setValue("nMethodIdx",pFinLoc.nMethodIdx);
			set.setValue("nEdge",pFinLoc.nEdge);
			set.setValue("fOpenSize",QString::number(pFinLoc.fOpenSize));
			set.setValue("nLowThres",pFinLoc.nLowThres);
			set.setValue("nHighThres",pFinLoc.nHighThres);
			set.setValue("nCenOffset",pFinLoc.nCenOffset);
			set.setValue("nFloatRange",pFinLoc.nFloatRange);
		}
		break;
	case ITEM_BASE_LOCATE:
		{
			s_pBaseLoc pBaseLoc = vItemPara.value<s_pBaseLoc>();
			set.setValue("name",pBaseLoc.strName);
			set.setValue("type",pBaseLoc.nType);
			set.setValue("bEnabled",pBaseLoc.bEnabled);
			set.setValue("checkRectShape",pBaseLoc.bRectShape);
			set.setValue("nMethodIdx",pBaseLoc.nMethodIdx);
			set.setValue("bIgnore",pBaseLoc.bIgnore);
			set.setValue("fSegRatio",QString::number(pBaseLoc.fSegRatio));
			set.setValue("nGray",pBaseLoc.nGray);
			set.setValue("nEdge",pBaseLoc.nEdge);
			set.setValue("nRadiusOffset",pBaseLoc.nRadiusOffset);
			set.setValue("nBeltSpace",pBaseLoc.nBeltSpace);
			set.setValue("nBeltWidth",pBaseLoc.nBeltWidth);
			set.setValue("nBeltHeight",pBaseLoc.nBeltHeight);
			set.setValue("nBeltEdge",pBaseLoc.nBeltEdge);
			set.setValue("nMoveOffset",pBaseLoc.nMoveOffset);
			set.setValue("nMoveStep",pBaseLoc.nMoveStep);
		}
		break;
	case ITEM_HORI_SIZE:
		{
			s_pHoriSize pHoriLoc = vItemPara.value<s_pHoriSize>();
			set.setValue("name",pHoriLoc.strName);
			set.setValue("type",pHoriLoc.nType);
			set.setValue("bEnabled",pHoriLoc.bEnabled);
			set.setValue("nEdge",pHoriLoc.nEdge);
			set.setValue("fCurValue",QString::number(pHoriLoc.fCurValue));
			set.setValue("fLower",QString::number(pHoriLoc.fLower));
			set.setValue("fUpper",QString::number(pHoriLoc.fUpper));
			set.setValue("fReal",QString::number(pHoriLoc.fReal));
			set.setValue("fModify",QString::number(pHoriLoc.fModify));
			if (!bCopy)//2014.6.23 复制参数时，不复制比例尺
			{
				set.setValue("fRuler",QString::number(pHoriLoc.fRuler));
			}
		}
		break;
	case ITEM_VERT_SIZE:
		{
			s_pVertSize pVertLoc = vItemPara.value<s_pVertSize>();
			set.setValue("name",pVertLoc.strName);
			set.setValue("type",pVertLoc.nType);
			set.setValue("bEnabled",pVertLoc.bEnabled);
			set.setValue("nEdge",pVertLoc.nEdge);
			set.setValue("fCurValue",QString::number(pVertLoc.fCurValue));
			set.setValue("fLower",QString::number(pVertLoc.fLower));
			set.setValue("fUpper",QString::number(pVertLoc.fUpper));
			set.setValue("fReal",QString::number(pVertLoc.fReal));
			set.setValue("fModify",QString::number(pVertLoc.fModify));
			if (!bCopy)//2014.6.23 复制参数时，不复制比例尺
			{
				set.setValue("fRuler",QString::number(pVertLoc.fRuler));
			}
		}
		break;
	case ITEM_FULL_HEIGHT:
		{
			s_pFullHeight pFullHeight = vItemPara.value<s_pFullHeight>();
			set.setValue("name",pFullHeight.strName);
			set.setValue("type",pFullHeight.nType);
			set.setValue("bEnabled",pFullHeight.bEnabled);
			set.setValue("nEdge",pFullHeight.nEdge);
			set.setValue("fCurValue",QString::number(pFullHeight.fCurValue));
			set.setValue("fLower",QString::number(pFullHeight.fLower));
			set.setValue("fUpper",QString::number(pFullHeight.fUpper));
			set.setValue("fReal",QString::number(pFullHeight.fReal));
			set.setValue("fModify",QString::number(pFullHeight.fModify));
			if (!bCopy)//2014.6.23 复制参数时，不复制比例尺
			{
				set.setValue("fRuler",QString::number(pFullHeight.fRuler));
			}
		}
		break;
	case ITEM_BENT_NECK:
		{
			s_pBentNeck pBentNeck = vItemPara.value<s_pBentNeck>();
			set.setValue("name",pBentNeck.strName);
			set.setValue("type",pBentNeck.nType);
			set.setValue("bEnabled",pBentNeck.bEnabled);
			set.setValue("nBentNeck",pBentNeck.nBentNeck);
		}
		break;
	case ITEM_VERT_ANG:
		{
			s_pVertAng pVertAng = vItemPara.value<s_pVertAng>();
			set.setValue("name",pVertAng.strName);
			set.setValue("type",pVertAng.nType);
			set.setValue("bEnabled",pVertAng.bEnabled);
			if (!bCopy)//2014.6.23 复制参数时，不复制比例尺
			{
				set.setValue("fRuler",QString::number(pVertAng.fRuler));
			}
			set.setValue("fVertAng",QString::number(pVertAng.fVertAng));
		}
		break;
	case ITEM_SGENNERAL_REGION:
		{
			s_pSGenReg pSGenReg = vItemPara.value<s_pSGenReg>();
			set.setValue("name",pSGenReg.strName);
			set.setValue("type",pSGenReg.nType);
			set.setValue("bEnabled",pSGenReg.bEnabled);
			set.setValue("nShapeType",pSGenReg.nShapeType);
			set.setValue("nEdge",pSGenReg.nEdge);
			set.setValue("nGray",pSGenReg.nGray);
			set.setValue("nArea",pSGenReg.nArea);
			set.setValue("nLength",pSGenReg.nLength);
			set.setValue("bStone",pSGenReg.bStone);
			set.setValue("nStoneEdge",pSGenReg.nStoneEdge);
			set.setValue("nStoneArea",pSGenReg.nStoneArea);
			set.setValue("nStoneNum",pSGenReg.nStoneNum);
			set.setValue("bDarkdot",pSGenReg.bDarkdot);
			set.setValue("nDarkdotEdge",pSGenReg.nDarkdotEdge);
			set.setValue("nDarkdotArea",pSGenReg.nDarkdotArea);
			set.setValue("fDarkdotCir",QString::number(pSGenReg.fDarkdotCir));
			set.setValue("nDarkdotNum",pSGenReg.nDarkdotNum);
			set.setValue("bTinyCrack",pSGenReg.bTinyCrack);
			set.setValue("nTinyCrackEdge",pSGenReg.nTinyCrackEdge);
			set.setValue("nTinyCrackAnsi",pSGenReg.nTinyCrackAnsi);
			set.setValue("nTinyCrackLength",pSGenReg.nTinyCrackLength);
			set.setValue("nTinyCrackInRadius",pSGenReg.nTinyCrackInRadius);
			set.setValue("nTinyCrackPhiL",pSGenReg.nTinyCrackPhiL);
			set.setValue("nTinyCrackPhiH",pSGenReg.nTinyCrackPhiH);
			set.setValue("bLightStripe",pSGenReg.bLightStripe);
			set.setValue("nLightStripeEdge",pSGenReg.nLightStripeEdge);
			set.setValue("nLightStripeLength",pSGenReg.nLightStripeLength);
			set.setValue("nLightStripeInRadius",pSGenReg.nLightStripeInRadius);
			set.setValue("nLightStripePhiL",pSGenReg.nLightStripePhiL);
			set.setValue("nLightStripePhiH",pSGenReg.nLightStripePhiH);
			set.setValue("bBubbles",pSGenReg.bBubbles);
			set.setValue("nBubblesLowThres",pSGenReg.nBubblesLowThres);
			set.setValue("nBubblesHighThres",pSGenReg.nBubblesHighThres);
			set.setValue("fBubblesCir",QString::number(pSGenReg.fBubblesCir));
			set.setValue("nBubblesLength",pSGenReg.nBubblesLength);
			//set.setValue("nBubblesArea",pSGenReg.nBubblesArea);
			//set.setValue("nBubblesGrayOffset",pSGenReg.nBubblesGrayOffset);

			set.setValue("bDistEdge",pSGenReg.bDistEdge);
			set.setValue("nDistEdge",pSGenReg.nDistEdge);
			set.setValue("bDistCon1",pSGenReg.bDistCon1);
			//set.setValue("nDistPhiL1",pSGenReg.nDistPhiL1);
			//set.setValue("nDistPhiH1",pSGenReg.nDistPhiH1);
			set.setValue("nDistVerPhi",pSGenReg.nDistVerPhi);
			set.setValue("nDistAniL1",pSGenReg.nDistAniL1);
			set.setValue("nDistAniH1",pSGenReg.nDistAniH1);
			set.setValue("nDistInRadiusL1",pSGenReg.nDistInRadiusL1);
			set.setValue("nDistInRadiusH1",pSGenReg.nDistInRadiusH1);
			set.setValue("bDistCon2",pSGenReg.bDistCon2);
			//set.setValue("nDistPhiL2",pSGenReg.nDistPhiL2);
			//set.setValue("nDistPhiH2",pSGenReg.nDistPhiH2);
			set.setValue("nDistHorPhi",pSGenReg.nDistHorPhi);
			set.setValue("nDistAniL2",pSGenReg.nDistAniL2);
			set.setValue("nDistAniH2",pSGenReg.nDistAniH2);
			set.setValue("nDistInRadiusL2",pSGenReg.nDistInRadiusL2);
			set.setValue("nDistInRadiusH2",pSGenReg.nDistInRadiusH2);

			set.setValue("bOpening",pSGenReg.bOpening);
			set.setValue("fOpeningSize",QString::number(pSGenReg.fOpeningSize));
			set.setValue("bGenRoi",pSGenReg.bGenRoi);
			set.setValue("fRoiRatio",QString::number(pSGenReg.fRoiRatio));
			set.setValue("nClosingWH",pSGenReg.nClosingWH);
			set.setValue("nGapWH",pSGenReg.nGapWH);
			set.setValue("nMaskSize",pSGenReg.nMaskSize);
			set.setValue("nMeanGray",pSGenReg.nMeanGray);
			set.setValue("fClosingSize",QString::number(pSGenReg.fClosingSize));
		}
		break;
	case ITEM_SSIDEFINISH_REGION:
		{
			s_pSSideFReg pSSideFReg = vItemPara.value<s_pSSideFReg>();
			set.setValue("name",pSSideFReg.strName);
			set.setValue("type",pSSideFReg.nType);
			set.setValue("bEnabled",pSSideFReg.bEnabled);
			set.setValue("nShapeType",pSSideFReg.nShapeType);
			set.setValue("nEdge",pSSideFReg.nEdge);		
			set.setValue("nArea",pSSideFReg.nArea);
			set.setValue("nWidth",pSSideFReg.nWidth);
			set.setValue("bGenRoi",pSSideFReg.bGenRoi);
			set.setValue("fRoiRatio",QString::number(pSSideFReg.fRoiRatio));
			set.setValue("nClosingWH",pSSideFReg.nClosingWH);
			set.setValue("nGapWH",pSSideFReg.nGapWH);
			set.setValue("nMaskSize",pSSideFReg.nMaskSize);
			set.setValue("fClosingSize",QString::number(pSSideFReg.fClosingSize));
		}
		break;
	case ITEM_SINFINISH_REGION:
		{
			s_pSInFReg pSInFReg = vItemPara.value<s_pSInFReg>();
			set.setValue("name",pSInFReg.strName);
			set.setValue("type",pSInFReg.nType);
			set.setValue("bEnabled",pSInFReg.bEnabled);
			set.setValue("nGray",pSInFReg.nGray);
			set.setValue("fOpeningSize",QString::number(pSInFReg.fOpeningSize));
			set.setValue("nArea",pSInFReg.nArea);
			set.setValue("nPos",pSInFReg.nPos);
		}
		break;
	case ITEM_SSCREWFINISH_REGION:
		{
			s_pSScrewFReg pSScrewFReg = vItemPara.value<s_pSScrewFReg>();
			set.setValue("name",pSScrewFReg.strName);
			set.setValue("type",pSScrewFReg.nType);
			set.setValue("bEnabled",pSScrewFReg.bEnabled);
			set.setValue("nShapeType",pSScrewFReg.nShapeType);
			set.setValue("nEdge",pSScrewFReg.nEdge);
			set.setValue("nArea",pSScrewFReg.nArea);
			set.setValue("nLength",pSScrewFReg.nLength);
			set.setValue("nDia",pSScrewFReg.nDia);
			set.setValue("nRab",pSScrewFReg.nRab);
		}
		break;
	case ITEM_FRLINNER_REGION:
		{
			s_pFRLInReg pFRLInReg = vItemPara.value<s_pFRLInReg>();
			set.setValue("name",pFRLInReg.strName);
			set.setValue("type",pFRLInReg.nType);
			set.setValue("bEnabled",pFRLInReg.bEnabled);
			set.setValue("nEdge",pFRLInReg.nEdge);
			set.setValue("nGray",pFRLInReg.nGray);
			set.setValue("nArea",pFRLInReg.nArea);
			set.setValue("nInnerRadius",pFRLInReg.nInnerRadius);
			set.setValue("fOpenSize",QString::number(pFRLInReg.fOpenSize));
			set.setValue("bOverPress",pFRLInReg.bOverPress);
			set.setValue("nPressEdge",pFRLInReg.nPressEdge);
			set.setValue("nPressNum",pFRLInReg.nPressNum);
			set.setValue("bInBroken",pFRLInReg.bInBroken);
			set.setValue("nBrokenEdge",pFRLInReg.nBrokenEdge);
			set.setValue("nBrokenArea",pFRLInReg.nBrokenArea);
			set.setValue("nBrokenGrayMean",pFRLInReg.nBrokenGrayMean);
		}
		break;
	case ITEM_FRLMIDDLE_REGION:
		{
			s_pFRLMidReg pFRLMidReg = vItemPara.value<s_pFRLMidReg>();
			set.setValue("name",pFRLMidReg.strName);
			set.setValue("type",pFRLMidReg.nType);
			set.setValue("bEnabled",pFRLMidReg.bEnabled);
			set.setValue("nEdge",pFRLMidReg.nEdge);
			set.setValue("nGray",pFRLMidReg.nGray);
			set.setValue("nArea",pFRLMidReg.nArea);
			set.setValue("nLen",pFRLMidReg.nLen);
			set.setValue("nEdge_2",pFRLMidReg.nEdge_2);
			set.setValue("nGray_2",pFRLMidReg.nGray_2);
			set.setValue("nArea_2",pFRLMidReg.nArea_2);
			set.setValue("nLen_2",pFRLMidReg.nLen_2);
			set.setValue("bOpen",pFRLMidReg.bOpen);
			set.setValue("fOpenSize",QString::number(pFRLMidReg.fOpenSize));
			set.setValue("bPitting",pFRLMidReg.bPitting);
			set.setValue("nPitEdge",pFRLMidReg.nPitEdge);
			set.setValue("nPitArea",pFRLMidReg.nPitArea);
			set.setValue("nPitNum",pFRLMidReg.nPitNum);
			set.setValue("bLOF",pFRLMidReg.bLOF);
			set.setValue("nLOFEdge",pFRLMidReg.nLOFEdge);
			set.setValue("fLOFRab",QString::number(pFRLMidReg.fLOFRab));
			set.setValue("nLOFLen",pFRLMidReg.nLOFLen);
			set.setValue("fLOFRab_In",QString::number(pFRLMidReg.fLOFRab_In));
			set.setValue("nLOFLen_In",pFRLMidReg.nLOFLen_In);
			set.setValue("fLOFRab_Out",QString::number(pFRLMidReg.fLOFRab_Out));
			set.setValue("nLOFLen_Out",pFRLMidReg.nLOFLen_Out);
			set.setValue("nLOFDiaMin",pFRLMidReg.nLOFDiaMin);
			set.setValue("nLOFDiaMax",pFRLMidReg.nLOFDiaMax);
			set.setValue("nLOFAngleOffset",pFRLMidReg.nLOFAngleOffset);
			set.setValue("bDeform",pFRLMidReg.bDeform);
			set.setValue("nDeformGary",pFRLMidReg.nDeformGary);
			set.setValue("nDeformHei",pFRLMidReg.nDeformHei);
			set.setValue("nDeformCirWid",pFRLMidReg.nDeformCirWid);
			set.setValue("bLOFOldWay",pFRLMidReg.bLOFOldWay);
			set.setValue("bLOFNewWay",pFRLMidReg.bLOFNewWay);
			set.setValue("nLOFEdge_new",pFRLMidReg.nLOFEdge_new);
			set.setValue("nLOFWidth_new",pFRLMidReg.nLOFWidth_new);
			set.setValue("nLOFLen_new",pFRLMidReg.nLOFLen_new);
		}
		break;
	case ITEM_FRLOUTER_REGION:
		{
			s_pFRLOutReg pFRLOutReg = vItemPara.value<s_pFRLOutReg>();
			set.setValue("name",pFRLOutReg.strName);
			set.setValue("type",pFRLOutReg.nType);
			set.setValue("bEnabled",pFRLOutReg.bEnabled);
			set.setValue("nEdge",pFRLOutReg.nEdge);
			set.setValue("nGray",pFRLOutReg.nGray);
			set.setValue("nArea",pFRLOutReg.nArea);
			set.setValue("fOpenSize",QString::number(pFRLOutReg.fOpenSize));
			set.setValue("bBreach",pFRLOutReg.bBreach);
			set.setValue("nBreachEdge",pFRLOutReg.nBreachEdge);
			set.setValue("nBreachWidth",pFRLOutReg.nBreachWidth);
			set.setValue("nBreachLen",pFRLOutReg.nBreachLen);
			set.setValue("bBrokenRing",pFRLOutReg.bBrokenRing);
			set.setValue("nBrokenRingEdge",pFRLOutReg.nBrokenRingEdge);
			set.setValue("nBrokenRingGray",pFRLOutReg.nBrokenRingGray);
			set.setValue("nBrokenRingLen",pFRLOutReg.nBrokenRingLen);
			set.setValue("fBrokenOpenSize",QString::number(pFRLOutReg.fBrokenOpenSize));	
		}
		break;
	case ITEM_FBLINNER_REGION:
		{
			s_pFBLInReg pFBLInReg = vItemPara.value<s_pFBLInReg>();
			set.setValue("name",pFBLInReg.strName);
			set.setValue("type",pFBLInReg.nType);
			set.setValue("bEnabled",pFBLInReg.bEnabled);
			set.setValue("nEdge",pFBLInReg.nEdge);
			set.setValue("nArea",pFBLInReg.nArea);
			set.setValue("fOpenSize",QString::number(pFBLInReg.fOpenSize));
			set.setValue("nLOFEdge",pFBLInReg.nLOFEdge);
			set.setValue("nLOFEdgeH",pFBLInReg.nLOFEdgeH);
			set.setValue("nLOFArea",pFBLInReg.nLOFArea);
			set.setValue("nLOFHeight",pFBLInReg.nLOFHeight);
			set.setValue("fLOFOpenSize",QString::number(pFBLInReg.fLOFOpenSize));
			set.setValue("nLOFPhiL",pFBLInReg.nLOFPhiL);
			set.setValue("nLOFPhiH",pFBLInReg.nLOFPhiH);
			set.setValue("nLOFMeanGray",pFBLInReg.nLOFMeanGray);
		}
		break;
	case ITEM_FBLMIDDLE_REGION:
		{
			s_pFBLMidReg pFBLMidReg = vItemPara.value<s_pFBLMidReg>();
			set.setValue("name",pFBLMidReg.strName);
			set.setValue("type",pFBLMidReg.nType);
			set.setValue("bEnabled",pFBLMidReg.bEnabled);
			set.setValue("nEdge",pFBLMidReg.nEdge);
			set.setValue("nArea",pFBLMidReg.nArea);
			set.setValue("bShadow",pFBLMidReg.bShadow);
			set.setValue("nShadowAng",pFBLMidReg.nShadowAng);
		}
		break;
	case ITEM_BINNER_REGION:
		{
			s_pBInReg pBInReg = vItemPara.value<s_pBInReg>();
			set.setValue("name",pBInReg.strName);
			set.setValue("type",pBInReg.nType);
			set.setValue("bEnabled",pBInReg.bEnabled);
			set.setValue("nMethodIdx",pBInReg.nMethodIdx);
			set.setValue("nEdge1",pBInReg.nEdge1);
			set.setValue("nArea1",pBInReg.nArea1);
			set.setValue("nOperation1",pBInReg.nOperation1);
			set.setValue("nLen1",pBInReg.nLen1);
			set.setValue("nMeanGray1",pBInReg.nMeanGray1);
			set.setValue("nEdge2",pBInReg.nEdge2);
			set.setValue("nArea2",pBInReg.nArea2);
			set.setValue("nOperation2",pBInReg.nOperation2);
			set.setValue("nLen2",pBInReg.nLen2);
			set.setValue("nMeanGray2",pBInReg.nMeanGray2);
			set.setValue("bOpen",pBInReg.bOpen);
			set.setValue("fOpenSize",QString::number(pBInReg.fOpenSize));
			set.setValue("bBottomDL",pBInReg.bBottomDL);
			set.setValue("nDarkB",pBInReg.nDarkB);
			set.setValue("nLightB",pBInReg.nLightB);
			set.setValue("bArc",pBInReg.bArc);
			set.setValue("nArcEdge",pBInReg.nArcEdge);
			set.setValue("nArcWidth",pBInReg.nArcWidth);
			set.setValue("bChar",pBInReg.bChar);
			set.setValue("nCharNum",pBInReg.nCharNum);
			set.setValue("fNeiRadius",QString::number(pBInReg.fNeiRadius));
			set.setValue("dModelPhi",QString::number(pBInReg.dModelPhi));
			set.setValue("bDoubleScale",pBInReg.bDoubleScale);
			set.setValue("nEdgeMin",pBInReg.nEdgeMin);
			set.setValue("nEdgeMax",pBInReg.nEdgeMax);
			set.setValue("nAreaMin",pBInReg.nAreaMin);
			set.setValue("fScaleRatio",QString::number(pBInReg.fScaleRatio));
			set.setValue("bSpot",pBInReg.bSpot);
			set.setValue("nAreaSpot",pBInReg.nAreaSpot);
			set.setValue("fSpotRound",QString::number(pBInReg.fSpotRound));
			set.setValue("nSpotNum",pBInReg.nSpotNum);
			set.setValue("bStrip",pBInReg.bStrip);
			set.setValue("nStripEdge",pBInReg.nStripEdge);
			set.setValue("nStripArea",pBInReg.nStripArea);
			set.setValue("nStripHeight",pBInReg.nStripHeight);
			set.setValue("nStripWidthL",pBInReg.nStripWidthL);
			set.setValue("nStripWidthH",pBInReg.nStripWidthH);
			set.setValue("fStripRab",QString::number(pBInReg.fStripRab));
			set.setValue("nStripAngleL",pBInReg.nStripAngleL);
			set.setValue("nStripAngleH",pBInReg.nStripAngleH);
		}
		break;
	case ITEM_BMIDDLE_REGION:
		{
			s_pBMidReg pBMidReg = vItemPara.value<s_pBMidReg>();
			set.setValue("name",pBMidReg.strName);
			set.setValue("type",pBMidReg.nType);
			set.setValue("bEnabled",pBMidReg.bEnabled);
			set.setValue("nEdge1",pBMidReg.nEdge1);
			set.setValue("nArea1",pBMidReg.nArea1);
			set.setValue("nOperation1",pBMidReg.nOperation1);
			set.setValue("nLen1",pBMidReg.nLen1);
			set.setValue("nMeanGray1",pBMidReg.nMeanGray1);
			set.setValue("nEdge2",pBMidReg.nEdge2);
			set.setValue("nArea2",pBMidReg.nArea2);
			set.setValue("nOperation2",pBMidReg.nOperation2);
			set.setValue("nLen2",pBMidReg.nLen2);
			set.setValue("nMeanGray2",pBMidReg.nMeanGray2);
			set.setValue("bOpen",pBMidReg.bOpen);
			set.setValue("fOpenSize",QString::number(pBMidReg.fOpenSize));
			set.setValue("bArc",pBMidReg.bArc);
			set.setValue("nArcEdge",pBMidReg.nArcEdge);
			set.setValue("nArcWidth",pBMidReg.nArcWidth);
		}
		break;
	case ITEM_BOUTER_REGION:
		{
			s_pBOutReg pBOutReg = vItemPara.value<s_pBOutReg>();
			set.setValue("name",pBOutReg.strName);
			set.setValue("type",pBOutReg.nType);
			set.setValue("bEnabled",pBOutReg.bEnabled);
			set.setValue("nEdge",pBOutReg.nEdge);
			set.setValue("nGray",pBOutReg.nGray);
			set.setValue("nArea",pBOutReg.nArea);
			set.setValue("nOperation",pBOutReg.nOperation);
			set.setValue("nLen",pBOutReg.nLen);
			set.setValue("nLenMax",pBOutReg.nLenMax);
			set.setValue("fOpenSize",QString::number(pBOutReg.fOpenSize));
			//检灌装线瓶底外环条纹
			set.setValue("bStrip",pBOutReg.bStrip);
			set.setValue("nStripEdge",pBOutReg.nStripEdge);
			set.setValue("nStripArea",pBOutReg.nStripArea);
			set.setValue("nStripHeight",pBOutReg.nStripHeight);
			set.setValue("nStripWidthL",pBOutReg.nStripWidthL);
			set.setValue("nStripWidthH",pBOutReg.nStripWidthH);
			set.setValue("fStripRab",QString::number(pBOutReg.fStripRab));
			set.setValue("nStripAngleL",pBOutReg.nStripAngleL);
			set.setValue("nStripAngleH",pBOutReg.nStripAngleH);
		}
		break;
	case ITEM_SSIDEWALL_REGION:
		{
			s_pSSideReg pSSideReg = vItemPara.value<s_pSSideReg>();
			set.setValue("name", pSSideReg.strName);
			//printf("%s",pSSideReg.strName);
			//printf("%s--%s--%s--%s,,,,","Savingpara:", pSSideReg.strName, QString::fromLocal8Bit(pSSideReg.strName),QString::fromUtf8(pSSideReg.strName));

			set.setValue("type",pSSideReg.nType);
			set.setValue("bEnabled",pSSideReg.bEnabled);
			set.setValue("nShapeType",pSSideReg.nShapeType);
			set.setValue("nEdge",pSSideReg.nEdge);
			set.setValue("nGray",pSSideReg.nGray);
			set.setValue("nArea",pSSideReg.nArea);
			set.setValue("fRab",QString::number(pSSideReg.fRab));
			set.setValue("bInspItem",pSSideReg.bInspItem);
			set.setValue("nSideDistance",pSSideReg.nSideDistance);

			set.setValue("bDistCon1",pSSideReg.bDistCon1);
			set.setValue("nDistVerPhi",pSSideReg.nDistVerPhi);
			set.setValue("nDistAniL1",pSSideReg.nDistAniL1);
			set.setValue("nDistAniH1",pSSideReg.nDistAniH1);
			set.setValue("nDistInRadiusL1",pSSideReg.nDistInRadiusL1);
			set.setValue("nDistInRadiusH1",pSSideReg.nDistInRadiusH1);
			set.setValue("bDistCon2",pSSideReg.bDistCon2);
			set.setValue("nDistHorPhi",pSSideReg.nDistHorPhi);
			set.setValue("nDistAniL2",pSSideReg.nDistAniL2);
			set.setValue("nDistAniH2",pSSideReg.nDistAniH2);
			set.setValue("nDistInRadiusL2",pSSideReg.nDistInRadiusL2);
			set.setValue("nDistInRadiusH2",pSSideReg.nDistInRadiusH2);
		}
		break;
	case ITEM_DISTURB_REGION:
		{
			s_pDistReg pDistReg = vItemPara.value<s_pDistReg>();
			set.setValue("name",pDistReg.strName);
			set.setValue("type",pDistReg.nType);
			set.setValue("bEnabled",pDistReg.bEnabled);
			set.setValue("nRegType",pDistReg.nRegType);
			set.setValue("nShapeType",pDistReg.nShapeType);
			set.setValue("bStripeSelf",pDistReg.bStripeSelf);
			set.setValue("fOpenSize",QString::number(pDistReg.fOpenSize));
			set.setValue("bIsMove",pDistReg.bIsMove);
			set.setValue("nStripeMaskSize",pDistReg.nStripeMaskSize);
			set.setValue("nStripeEdge",pDistReg.nStripeEdge);
			set.setValue("bVertStripe",pDistReg.bVertStripe);
			set.setValue("nVertAng",pDistReg.nVertAng);
			set.setValue("nVertWidthL",pDistReg.nVertWidthL);
			set.setValue("nVertWidthH",pDistReg.nVertWidthH);
			set.setValue("fVertRabL",QString::number(pDistReg.fVertRabL));
			set.setValue("fVertRabH",QString::number(pDistReg.fVertRabH));
			set.setValue("fVertInRadiusL",QString::number(pDistReg.fVertInRadiusL));
			set.setValue("fVertInRadiusH",QString::number(pDistReg.fVertInRadiusH));
			set.setValue("bHoriStripe",pDistReg.bHoriStripe);
			set.setValue("nHoriAng",pDistReg.nHoriAng);
			set.setValue("nHoriWidthL",pDistReg.nHoriWidthL);
			set.setValue("nHoriWidthH",pDistReg.nHoriWidthH);
			set.setValue("fHoriRabL",QString::number(pDistReg.fHoriRabL));
			set.setValue("fHoriRabH",QString::number(pDistReg.fHoriRabH));
			set.setValue("fHoriInRadiusL",QString::number(pDistReg.fHoriInRadiusL));
			set.setValue("fHoriInRadiusH",QString::number(pDistReg.fHoriInRadiusH));
			set.setValue("fBubbleCir",QString::number(pDistReg.fBubbleCir));
			set.setValue("nBubbleArea",QString::number(pDistReg.nBubbleArea));
			set.setValue("nBubbleLowThre",QString::number(pDistReg.nBubbleLowThre));
			set.setValue("nBubbleHighThre",QString::number(pDistReg.nBubbleHighThre));
		}
		break;
	case ITEM_FINISH_CONTOUR:
		{
			s_pFinCont pFinCont = vItemPara.value<s_pFinCont>();
			set.setValue("name",pFinCont.strName);
			set.setValue("type",pFinCont.nType);
			set.setValue("bEnabled",pFinCont.bEnabled);
			set.setValue("nSubThresh",pFinCont.nSubThresh);
			set.setValue("nRegWidth",pFinCont.nRegWidth);
			set.setValue("nRegHeight",pFinCont.nRegHeight);
			set.setValue("nGapWidth",pFinCont.nGapWidth);
			set.setValue("nGapHeight",pFinCont.nGapHeight);
			set.setValue("nNotchHeight",pFinCont.nNotchHeight);
			set.setValue("nArea",pFinCont.nArea);
			set.setValue("bCheckOverPress",pFinCont.bCheckOverPress);
		}
		break;
	case ITEM_NECK_CONTOUR:
		{
			s_pNeckCont pNeckCont = vItemPara.value<s_pNeckCont>();
			set.setValue("name",pNeckCont.strName);
			set.setValue("type",pNeckCont.nType);
			set.setValue("bEnabled",pNeckCont.bEnabled);
			set.setValue("nThresh",pNeckCont.nThresh);
			set.setValue("nNeckHeiL",pNeckCont.nNeckHeiL);
			set.setValue("nNeckHeiH",pNeckCont.nNeckHeiH);
			set.setValue("nArea",pNeckCont.nArea);
		}
		break;
	case ITEM_BODY_CONTOUR:
		{
			s_pBodyCont pBodyCont = vItemPara.value<s_pBodyCont>();
			set.setValue("name",pBodyCont.strName);
			set.setValue("type",pBodyCont.nType);
			set.setValue("bEnabled",pBodyCont.bEnabled);
			set.setValue("nThresh",pBodyCont.nThresh);
			set.setValue("nWidth",pBodyCont.nWidth);
			set.setValue("nArea",pBodyCont.nArea);
		}
		break;
	case ITEM_SBRISPOT_REGION:
		{
			s_pSBriSpotReg pSBriSpotReg = vItemPara.value<s_pSBriSpotReg>();
			set.setValue("name",pSBriSpotReg.strName);
			set.setValue("type",pSBriSpotReg.nType);
			set.setValue("bEnabled",pSBriSpotReg.bEnabled);
			set.setValue("nShapeType",pSBriSpotReg.nShapeType);
			set.setValue("nGray",pSBriSpotReg.nGray);
			set.setValue("fOpenRadius",QString::number(pSBriSpotReg.fOpenRadius));
			set.setValue("nArea",pSBriSpotReg.nArea);
		}
		break;
	case ITEM_BALL_REGION:
		{
			s_pBAllReg pBAllReg = vItemPara.value<s_pBAllReg>();
			set.setValue("name",pBAllReg.strName);
			set.setValue("type",pBAllReg.nType);
			set.setValue("bEnabled",pBAllReg.bEnabled);

			set.setValue("bModeNO",pBAllReg.bModeNO);
			set.setValue("nModePointEdge",pBAllReg.nModePointEdge);
			set.setValue("nModeNOWidth",pBAllReg.nModeNOWidth);
			set.setValue("nModeNOHeight",pBAllReg.nModeNOHeight);
			set.setValue("nModePointNum",pBAllReg.nModePointNum);
			set.setValue("fModePointRadius",QString::number(pBAllReg.fModePointRadius));
			set.setValue("nMaxModePointSpace",pBAllReg.nMaxModePointSpace);

			set.setValue("nMaskWidth",pBAllReg.nMaskWidth);
			set.setValue("nMaskHeight",pBAllReg.nMaskHeight);
			set.setValue("nFontCloseScale",pBAllReg.nFontCloseScale);
			set.setValue("nModePointSobel",pBAllReg.nModePointSobel);
			set.setValue("nMinModePointHeight",pBAllReg.nMinModePointHeight);
			set.setValue("fSobelOpeningScale",QString::number(pBAllReg.fSobelOpeningScale));

			set.setValue("bModeNOExt",pBAllReg.bModeNOExt);
			set.setValue("nMinModePointNumExt",pBAllReg.nMinModePointNumExt);
			set.setValue("nSigPointDisExt",pBAllReg.nSigPointDisExt);
			
			set.value("bDelStripe",pBAllReg.bDelStripe);
			set.value("nStrLengthth",pBAllReg.nStrLengthth);
			set.value("nStrWidth",pBAllReg.nStrWidth);
			set.value("Checkthirtmp",pBAllReg.bFlagThirtMp);
			set.setValue("nMouldEdge",pBAllReg.nMouldEdge);
			set.setValue("nMouldDia",pBAllReg.nMouldDia);
			set.setValue("nMouldSpace",pBAllReg.nMouldSpace);
			set.setValue("bCheckAntiClockwise",pBAllReg.bCheckAntiClockwise);
			set.setValue("nMouldInnerDiaL",pBAllReg.nMouldInnerDiaL);
			set.setValue("nMouldInnerDiaH",pBAllReg.nMouldInnerDiaH);
			set.setValue("bCheckIfReportError",pBAllReg.bCheckIfReportError);
			set.setValue("nModeReject_1",pBAllReg.nModeReject_1);
			set.setValue("nModeReject_2",pBAllReg.nModeReject_2);
			set.setValue("nModeReject_3",pBAllReg.nModeReject_3);
			set.setValue("strModeReject",pBAllReg.strModeReject);
		}
		break;
	case ITEM_CIRCLE_SIZE:
		{
			s_pCirSize pCirSize = vItemPara.value<s_pCirSize>();
			set.setValue("name",pCirSize.strName);
			set.setValue("type",pCirSize.nType);
			set.setValue("bEnabled",pCirSize.bEnabled);
			set.setValue("nEdge",pCirSize.nEdge);
			set.setValue("bDia",pCirSize.bDia);
			set.setValue("fDiaCurValue",QString::number(pCirSize.fDiaCurValue));
			set.setValue("fDiaLower",QString::number(pCirSize.fDiaLower));
			set.setValue("fDiaUpper",QString::number(pCirSize.fDiaUpper));
			set.setValue("fDiaModify",QString::number(pCirSize.fDiaModify));
			set.setValue("fDiaReal",QString::number(pCirSize.fDiaReal));
			set.setValue("bOvality",pCirSize.bOvality);
			set.setValue("fOvalCurValue",QString::number(pCirSize.fOvalCurValue));
			set.setValue("fOvality",QString::number(pCirSize.fOvality));
			if (!bCopy)//2014.6.23 复制参数时，不复制比例尺
			{
				set.setValue("fDiaRuler",QString::number(pCirSize.fDiaRuler));
			}
		}
		break;
	case ITEM_SBASE_REGION:
		{
			s_pSBaseReg pSBaseReg = vItemPara.value<s_pSBaseReg>();
			set.setValue("name",pSBaseReg.strName);
			set.setValue("type",pSBaseReg.nType);
			set.setValue("bEnabled",pSBaseReg.bEnabled);
			set.setValue("nMethodIdx",pSBaseReg.nMethodIdx);
			set.setValue("nEdge",pSBaseReg.nEdge);
			set.setValue("nGray",pSBaseReg.nGray);
			set.setValue("nArea",pSBaseReg.nArea);
			set.setValue("fRab",QString::number(pSBaseReg.fRab));
			set.setValue("nSideDistance",pSBaseReg.nSideDistance);
		}
		break;
	case ITEM_SBASECONVEX_REGION:
		{
			s_pSBaseConvexReg pSBaseConvexReg = vItemPara.value<s_pSBaseConvexReg>();
			set.setValue("name",pSBaseConvexReg.strName);
			set.setValue("type",pSBaseConvexReg.nType);
			set.setValue("bEnabled",pSBaseConvexReg.bEnabled);
			set.setValue("bGenDefects",pSBaseConvexReg.bGenDefects);
			set.setValue("nEdge",pSBaseConvexReg.nEdge);
			set.setValue("nGray",pSBaseConvexReg.nGray);
			set.setValue("nArea",pSBaseConvexReg.nArea);
			set.setValue("nLength",pSBaseConvexReg.nLength);
			set.setValue("bContDefects",pSBaseConvexReg.bContDefects);
			set.setValue("nContGray",pSBaseConvexReg.nContGray);
			set.setValue("nContArea",pSBaseConvexReg.nContArea);
			set.setValue("nContLength",pSBaseConvexReg.nContLength);
			set.setValue("fOpeningSize",QString::number(pSBaseConvexReg.fOpeningSize));

			set.setValue("bGenRoi",pSBaseConvexReg.bGenRoi);
			set.setValue("fRoiRatio",QString::number(pSBaseConvexReg.fRoiRatio));
			set.setValue("nClosingWH",pSBaseConvexReg.nClosingWH);
			set.setValue("nGapWH",pSBaseConvexReg.nGapWH);
			set.setValue("nMaskSize",pSBaseConvexReg.nMaskSize);
			set.setValue("fClosingSize",QString::number(pSBaseConvexReg.fClosingSize));
		}
		break;
	default:
		break;
	}
}
//*功能：根据类型，保存形状数据
void CCheck::saveShapebyType(int nType,QSettings &set,QVariant &vItemShape)
{
	switch(nType)
	{
	case ITEM_SIDEWALL_LOCATE:
		{
			HTuple tpRow,tpCol;
			int num;
			s_oSideLoc oSideLoc = vItemShape.value<s_oSideLoc>();
			set.beginGroup("offset");
			set.setValue("drow1",QString::number(oSideLoc.drow1));
			set.setValue("dcol1",QString::number(oSideLoc.dcol1));
			set.setValue("dphi1",QString::number(oSideLoc.dphi1));
			set.setValue("drow2",QString::number(oSideLoc.drow2));
			set.setValue("dcol2",QString::number(oSideLoc.dcol2));
			set.setValue("dphi2",QString::number(oSideLoc.dphi2));
			set.setValue("nLeftRight",oSideLoc.nLeftRight);
			set.endGroup();

			set.beginGroup("origin");
			set.setValue("Row",QString::number(oSideLoc.ori.Row));
			set.setValue("Col",QString::number(oSideLoc.ori.Col));
			set.setValue("Angle",QString::number(oSideLoc.ori.Angle));
			set.setValue("nRow11",oSideLoc.ori.nRow11);
			set.setValue("nCol11",oSideLoc.ori.nCol11);
			set.setValue("nRow12",oSideLoc.ori.nRow12);
			set.setValue("nCol12",oSideLoc.ori.nCol12);
			set.setValue("nRow21",oSideLoc.ori.nRow21);
			set.setValue("nCol21",oSideLoc.ori.nCol21);
			set.setValue("nRow22",oSideLoc.ori.nRow22);
			set.setValue("nCol22",oSideLoc.ori.nCol22);
			set.setValue("fDist1",QString::number(oSideLoc.ori.fDist1));
			set.setValue("fDist2",QString::number(oSideLoc.ori.fDist2));
			set.setValue("fDist3",QString::number(oSideLoc.ori.fDist3));
			set.endGroup();
			set.beginGroup("firstLine");			
			get_region_points(oSideLoc.oFirstLine,&tpRow,&tpCol);
			num = tpRow.Num();
			if (tpCol[0].I()>tpCol[num-1].I())
			{
				tpRow = tpRow.Inverse();//防止某些方向时起始点颠倒
				tpCol = tpCol.Sort();
			}
			set.setValue("x0",tpCol[0].I());
			set.setValue("y0",tpRow[0].I());
			set.setValue("x1",tpCol[num-1].I());
			set.setValue("y1",tpRow[num-1].I());
			set.endGroup();
			set.beginGroup("secondLine");
			get_region_points(oSideLoc.oSecondLine,&tpRow,&tpCol);
			num = tpRow.Num();
			if (tpCol[0].I()>tpCol[num-1].I())
			{
				tpRow = tpRow.Inverse();
				tpCol = tpCol.Sort();
			}
			set.setValue("x0",tpCol[0].I());
			set.setValue("y0",tpRow[0].I());
			set.setValue("x1",tpCol[num-1].I());
			set.setValue("y1",tpRow[num-1].I());
			set.endGroup();
			set.beginGroup("thirdLine");
			get_region_points(oSideLoc.oThirdLine,&tpRow,&tpCol);
			num = tpRow.Num();
			if (tpCol[0].I()>tpCol[num-1].I())
			{
				tpRow = tpRow.Inverse();
				tpCol = tpCol.Sort();
			}
			set.setValue("x0",tpCol[0].I());
			set.setValue("y0",tpRow[0].I());
			set.setValue("x1",tpCol[num-1].I());
			set.setValue("y1",tpRow[num-1].I());
			set.endGroup();
		}		
		break;
	case ITEM_FINISH_LOCATE:
		{
			double fRow,fCol,fRadius;
			s_oFinLoc oFinLoc = vItemShape.value<s_oFinLoc>();
			set.beginGroup("origin");
			set.setValue("Row",QString::number(oFinLoc.Row));
			set.setValue("Col",QString::number(oFinLoc.Col));
			set.setValue("Radius",QString::number(oFinLoc.Radius));
			set.endGroup();
			set.beginGroup("inCircle");
			smallest_circle(oFinLoc.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",fRow);
			set.setValue("fCol",fCol);
			set.setValue("fRadius",fRadius);
			set.endGroup();
			set.beginGroup("outCircle");
			smallest_circle(oFinLoc.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",fRow);
			set.setValue("fCol",fCol);
			set.setValue("fRadius",fRadius);
			set.endGroup();
		}
		break;
	case ITEM_BASE_LOCATE:
		{
			double fRow,fCol,fRadius;
			s_oBaseLoc oBaseLoc = vItemShape.value<s_oBaseLoc>();
			set.beginGroup("offset");
			set.setValue("drow1",QString::number(oBaseLoc.drow1));
			set.setValue("dcol1",QString::number(oBaseLoc.dcol1));
			set.setValue("dradius1",QString::number(oBaseLoc.dradius1));
			set.setValue("drow2",QString::number(oBaseLoc.drow2));
			set.setValue("dcol2",QString::number(oBaseLoc.dcol2));
			set.setValue("dradius2",QString::number(oBaseLoc.dradius2));
			set.endGroup();
			set.beginGroup("origin");
			set.setValue("Row",QString::number(oBaseLoc.Row));
			set.setValue("Col",QString::number(oBaseLoc.Col));
			set.setValue("Radius",QString::number(oBaseLoc.Radius));
			set.setValue("Angle",QString::number(oBaseLoc.Angle));
			set.endGroup();
			set.beginGroup("centReg");
			smallest_circle(oBaseLoc.oCentReg,&fRow,&fCol,&fRadius);
			set.setValue("fRow",fRow);
			set.setValue("fCol",fCol);
			set.setValue("fRadius",fRadius);
			set.endGroup();
			set.beginGroup("oModeNOEdge");
			smallest_circle(oBaseLoc.oModeNOEdge,&fRow,&fCol,&fRadius);
			set.setValue("fRow",fRow);
			set.setValue("fCol",fCol);
			set.setValue("fRadius",fRadius);
			set.endGroup();
			set.beginGroup("beltDia");
			smallest_circle(oBaseLoc.oBeltDia,&fRow,&fCol,&fRadius);
			set.setValue("fRow",fRow);
			set.setValue("fCol",fCol);
			set.setValue("fRadius",fRadius);
			set.endGroup();

			double row2,col2;
			set.beginGroup("beltDia_Rect");
			smallest_rectangle1_xld(oBaseLoc.oBeltDia_Rect,&fRow,&fCol,&row2,&col2);
			set.setValue("row1",fRow);
			set.setValue("col1",fCol);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();	

		/*	set.beginGroup("oModeNOEdge_Rect");
			smallest_rectangle1_xld(oBaseLoc.oModeNOEdge_Rect,&fRow,&fCol,&row2,&col2);
			set.setValue("row1",fRow);
			set.setValue("col1",fCol);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();	

			set.beginGroup("oCentReg_Rect");
			smallest_rectangle1_xld(oBaseLoc.oCentReg_Rect,&fRow,&fCol,&row2,&col2);
			set.setValue("row1",fRow);
			set.setValue("col1",fCol);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();*/
		}
		break;
	case ITEM_HORI_SIZE:
		{
			double row1,col1,row2,col2;
			s_oHoriSize oHoriSize = vItemShape.value<s_oHoriSize>();
			set.beginGroup("ptLeft");
			set.setValue("x",oHoriSize.ptLeft.x());
			set.setValue("y",oHoriSize.ptLeft.y());
			set.endGroup();
			set.beginGroup("ptRight");
			set.setValue("x",oHoriSize.ptRight.x());
			set.setValue("y",oHoriSize.ptRight.y());
			set.endGroup();
			set.beginGroup("sizeRect");
			smallest_rectangle1_xld(oHoriSize.oSizeRect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_VERT_SIZE:
		{
			double row1,col1,row2,col2;
			s_oVertSize oVertSize = vItemShape.value<s_oVertSize>();
			set.beginGroup("ptLeft");
			set.setValue("x",oVertSize.ptLeft.x());
			set.setValue("y",oVertSize.ptLeft.y());
			set.endGroup();
			set.beginGroup("ptRight");
			set.setValue("x",oVertSize.ptRight.x());
			set.setValue("y",oVertSize.ptRight.y());
			set.endGroup();
			set.beginGroup("sizeRect");
			smallest_rectangle1_xld(oVertSize.oSizeRect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_FULL_HEIGHT:
		{
			double row1,col1,row2,col2;
			s_oFullHeight oFullHeight = vItemShape.value<s_oFullHeight>();
			set.beginGroup("ptLeft");
			set.setValue("x",oFullHeight.ptLeft.x());
			set.setValue("y",oFullHeight.ptLeft.y());
			set.endGroup();
			set.beginGroup("ptRight");
			set.setValue("x",oFullHeight.ptRight.x());
			set.setValue("y",oFullHeight.ptRight.y());
			set.endGroup();
			set.beginGroup("sizeRect");
			smallest_rectangle1_xld(oFullHeight.oSizeRect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_BENT_NECK:
		{
			double row1,col1,row2,col2;
			s_oBentNeck oBentNeck = vItemShape.value<s_oBentNeck>();
			set.beginGroup("finRect");
			smallest_rectangle1_xld(oBentNeck.oFinRect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();			
		}
		break;
	case ITEM_VERT_ANG:
		{			
			s_oVertAng oVertAng = vItemShape.value<s_oVertAng>();			
		}
		break;
	case ITEM_SGENNERAL_REGION:
		{	
			s_oSGenReg oSGenReg = vItemShape.value<s_oSGenReg>();
			set.beginGroup("checkRegion");
			saveXLDbyPts(set,oSGenReg.oCheckRegion);
			set.endGroup();

			// 2017.2
			double row1,col1,row2,col2;
			set.beginGroup("checkRegion_Rect");
			smallest_rectangle1_xld(oSGenReg.oCheckRegion_Rect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();	

		}
		break;
	case ITEM_DISTURB_REGION:
		{
			s_oDistReg oDistReg = vItemShape.value<s_oDistReg>();
			set.beginGroup("disturbReg");
			saveXLDbyPts(set,oDistReg.oDisturbReg);
			set.endGroup();
			// 2017.11
			double row1,col1,row2,col2;
			set.beginGroup("disturbReg_Rect");
			smallest_rectangle1_xld(oDistReg.oDisturbReg_Rect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_SSIDEFINISH_REGION:
		{	
			s_oSSideFReg oSSideFReg = vItemShape.value<s_oSSideFReg>();
			set.beginGroup("checkRegion");
			saveXLDbyPts(set,oSSideFReg.oCheckRegion);
			set.endGroup();
			// 2017.11
			double row1,col1,row2,col2;
			set.beginGroup("checkRegion_Rect");
			smallest_rectangle1_xld(oSSideFReg.oCheckRegion_Rect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_SINFINISH_REGION:
		{	
			s_oSInFReg oSInFReg = vItemShape.value<s_oSInFReg>();
			set.beginGroup("checkRegion");
			saveXLDbyPts(set,oSInFReg.oCheckRegion);
			set.endGroup();
		}
		break;
	case ITEM_SSCREWFINISH_REGION:
		{	
			s_oSScrewFReg oSScrewFReg = vItemShape.value<s_oSScrewFReg>();
			set.beginGroup("checkRegion");
			saveXLDbyPts(set,oSScrewFReg.oCheckRegion);
			set.endGroup();
			// 2017.11
			double row1,col1,row2,col2;
			set.beginGroup("checkRegion_Rect");
			smallest_rectangle1_xld(oSScrewFReg.oCheckRegion_Rect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_FRLINNER_REGION:
		{	
			s_oFRLInReg oFRLInReg = vItemShape.value<s_oFRLInReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oFRLInReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oFRLInReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;
	case ITEM_FRLMIDDLE_REGION:
		{	
			s_oFRLMidReg oFRLMidReg = vItemShape.value<s_oFRLMidReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oFRLMidReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oFRLMidReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;
	case ITEM_FRLOUTER_REGION:
		{	
			s_oFRLOutReg oFRLOutReg = vItemShape.value<s_oFRLOutReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oFRLOutReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oFRLOutReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;
	case ITEM_FBLINNER_REGION:
		{	
			s_oFBLInReg oFBLInReg = vItemShape.value<s_oFBLInReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oFBLInReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oFBLInReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			// 2017.3
			set.beginGroup("oPolygon");
			saveXLDbyPts(set,oFBLInReg.oPolygon);
			set.endGroup();
		}
		break;
	case ITEM_FBLMIDDLE_REGION:
		{	
			s_oFBLMidReg oFBLMidReg = vItemShape.value<s_oFBLMidReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oFBLMidReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oFBLMidReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;
	case ITEM_BINNER_REGION:
		{	
			s_oBInReg oBInReg = vItemShape.value<s_oBInReg>();
			set.beginGroup("oTriBase");
			saveXLDbyPts(set,oBInReg.oTriBase);
			set.endGroup();

			set.beginGroup("oRectBase");
			saveXLDbyPts(set,oBInReg.oRectBase);
			set.endGroup();

			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oBInReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();		
			set.beginGroup("oOutCircle");
			smallest_circle(oBInReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oMarkReg");
			smallest_circle(oBInReg.oMarkReg,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oCharReg");
			saveXLDbyPts(set,oBInReg.oCharReg);
			set.endGroup();
			m_nBaseCharModelID = oBInReg.ModelID;
		}
		break;
	case ITEM_BMIDDLE_REGION:
		{	
			s_oBMidReg oBMidReg = vItemShape.value<s_oBMidReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oBMidReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oBMidReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;
	case ITEM_BOUTER_REGION:
		{	
			s_oBOutReg oBOutReg = vItemShape.value<s_oBOutReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oBOutReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oBOutReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;
	case ITEM_SSIDEWALL_REGION:
		{	
			s_oSSideReg oSSideReg = vItemShape.value<s_oSSideReg>();
			set.beginGroup("checkRegion");
			saveXLDbyPts(set,oSSideReg.oCheckRegion);
			set.endGroup();
			// 2017.11
			double row1,col1,row2,col2;
			set.beginGroup("checkRegion_Rect");
			smallest_rectangle1_xld(oSSideReg.oCheckRegion_Rect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_FINISH_CONTOUR:
		{	
			s_oFinCont oFinCont = vItemShape.value<s_oFinCont>();			
		}
		break;
	case ITEM_NECK_CONTOUR:
		{
			double row1,col1,row2,col2;
			s_oNeckCont oNeckCont = vItemShape.value<s_oNeckCont>();			
			set.beginGroup("checkRegion");
			smallest_rectangle1_xld(oNeckCont.oCheckRegion,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_BODY_CONTOUR:
		{
			double row1,col1,row2,col2;
			s_oBodyCont oBodyCont = vItemShape.value<s_oBodyCont>();			
			set.beginGroup("checkRegion");
			//smallest_rectangle1(oBodyCont.oCheckRegion,&row1,&col1,&row2,&col2);
			smallest_rectangle1_xld(oBodyCont.oCheckRegion,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;	
	case ITEM_SBRISPOT_REGION:
		{
			s_oSBriSpotReg oSBriSpotReg = vItemShape.value<s_oSBriSpotReg>();			
			set.beginGroup("checkRegion");
			saveXLDbyPts(set,oSBriSpotReg.oCheckRegion);
			set.endGroup();
			// 2017.11
			double row1,col1,row2,col2;
			set.beginGroup("checkRegion_Rect");
			smallest_rectangle1_xld(oSBriSpotReg.oCheckRegion_Rect,&row1,&col1,&row2,&col2);
			set.setValue("row1",row1);
			set.setValue("col1",col1);
			set.setValue("row2",row2);
			set.setValue("col2",col2);
			set.endGroup();
		}
		break;
	case ITEM_BALL_REGION:
		{
			s_oBAllReg oBAllReg = vItemShape.value<s_oBAllReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oBAllReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oBAllReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;	
	case ITEM_CIRCLE_SIZE:
		{	
			s_oCirSize oCirSize = vItemShape.value<s_oCirSize>();
			double fRow,fCol,fRadius;
			set.beginGroup("oCircle");
			smallest_circle(oCirSize.oCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;
	case ITEM_SBASE_REGION:
		{	
			s_oSBaseReg oSBaseReg = vItemShape.value<s_oSBaseReg>();
			double fRow,fCol,fRadius;
			set.beginGroup("oInCircle");
			smallest_circle(oSBaseReg.oInCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
			set.beginGroup("oOutCircle");
			smallest_circle(oSBaseReg.oOutCircle,&fRow,&fCol,&fRadius);
			set.setValue("fRow",QString::number(fRow));
			set.setValue("fCol",QString::number(fCol));
			set.setValue("fRadius",QString::number(fRadius));
			set.endGroup();
		}
		break;	
	case ITEM_SBASECONVEX_REGION:
		{	
			s_oSBaseConvexReg oSBaseConvexReg = vItemShape.value<s_oSBaseConvexReg>();
			set.beginGroup("checkRegion");
			saveXLDbyPts(set,oSBaseConvexReg.oCheckRegion);
			set.endGroup();
		}
		break;
	default:
		break;
	}
}
//*功能：从模板文件中读入节点生成XLD
void CCheck::readXLDbyPts(QSettings &set,Hobject *oXLD)
{
	int i;
	HTuple tpRow,tpCol;
	QVector<QPoint> vAllPts;
	int nPtNum = set.value("nPtNum",0).toInt();
	if (nPtNum < 3)//生成默认区域
	{
		gen_contour_polygon_xld(oXLD,
			HTuple(100).Concat(57).Concat(57).Concat(100).Concat(143).Concat(143).Concat(100),
			HTuple(50).Concat(75).Concat(125).Concat(150).Concat(125).Concat(75).Concat(50));
	} 
	else
	{
		//读入点
		vAllPts.clear();
		for (i=0;i<nPtNum;++i)
		{
			QString str = QString("pt%1").arg(i);
			set.beginGroup(str);
			int row = set.value("row").toInt();
			int col = set.value("col").toInt();
			vAllPts.push_back(QPoint(row,col));
			set.endGroup();
		}
		//点生成区域
		tpRow = tpCol = HTuple();
		QVector<QPoint>::const_iterator startPt,endPt;
		startPt = vAllPts.begin();
		endPt = vAllPts.end();
		for (;startPt!=endPt;++startPt)
		{
			tuple_concat(tpCol,HTuple(startPt->y()),&tpCol);
			tuple_concat(tpRow,HTuple(startPt->x()),&tpRow);
		}
		gen_contour_polygon_xld(oXLD,tpRow,tpCol);
	}	
}
//*功能：将XLD转为节点存入模板文件
void CCheck::saveXLDbyPts(QSettings &set,Hobject &oXLD)
{
	HTuple tpRow,tpCol,tpLen;
	int i,nLen;
	get_contour_xld(oXLD,&tpRow,&tpCol);
	tuple_length(tpRow,&tpLen);
	nLen = tpLen[0].I();
	if (nLen==0)
		return;
	set.setValue("nPtNum",nLen);
	for (i=0;i<nLen;++i)
	{
		QString str = QString("pt%1").arg(i);
		set.beginGroup(str);
		set.setValue("row",tpRow[i].I());
		set.setValue("col",tpCol[i].I());
		set.endGroup();
	}
}
//*功能：将XLD转为主骨架点
void CCheck::XLDToPts(Hobject &oXLD, s_Xld_Point & xldPoint )
{
	try
	{
		HTuple tpRow,tpCol,tpLen;
		int i,j,nLen;
		float fstep;
		get_contour_xld(oXLD,&tpRow,&tpCol);
		tuple_length(tpRow,&tpLen);
		xldPoint.nCount = 0;
		nLen = tpLen[0].I();
		if (nLen<BOTTLEXLD_POINTNUM)
		{
			return;
		}
		fstep = 1.0*nLen/BOTTLEXLD_POINTNUM;
		for (i=0,j=0;i<BOTTLEXLD_POINTNUM-1;++i)
		{
			j=i*fstep;
			xldPoint.nRowsAry[i] = tpRow[j].I();
			xldPoint.nColsAry[i] = tpCol[j].I();
		}
		xldPoint.nRowsAry[i] = tpRow[nLen-1].I();
		xldPoint.nColsAry[i] = tpCol[nLen-1].I();
		xldPoint.nCount = i+1;
	}
	catch (...)
	{
		writeErrDataRecord(QString("abnormal:XLDToPts,XLD convert to point_group"));	//轮廓转换点集	
	}
}
//*功能：将节点转为XLD
void CCheck::PtsToXLD(s_Xld_Point & xldPoint,Hobject &oXLD)
{
	try
	{
		HTuple tpRow,tpCol;
		tpRow = tpCol = HTuple();
		gen_empty_obj(&oXLD);
		if (xldPoint.nCount<3 || xldPoint.nCount>BOTTLEXLD_POINTNUM)
		{
			return;
		}
		//点生成区域
		for (int i = 0;i<xldPoint.nCount;i++)
		{
			tuple_concat(tpCol,HTuple(xldPoint.nColsAry[i]),&tpCol);
			tuple_concat(tpRow,HTuple(xldPoint.nRowsAry[i]),&tpRow);
		}
		gen_contour_polygon_xld(&oXLD,tpRow,tpCol);
	}
	catch (...)
	{
		writeErrDataRecord(QString("abnormal:PtsToXLD,Point_group convert to XLD"));	//点集转换轮廓	
	}
}
//*功能：多边形区域仿射变换,iMethod不旋转时有效
void CCheck::affineRegion(Hobject &oRegion,bool bRotate /* = false */,int iMethod /* = 0 */)
{
	if (currentOri.Row == 0 && currentOri.Col ==0)
	{
		return;//没有定位
	}
	if (bRotate)//平移旋转（xld）
	{
		HTuple homMat2DIdentity;
		hom_mat2d_identity(&homMat2DIdentity);
		vector_angle_to_rigid(modelOri.Row,modelOri.Col,modelOri.Angle,
			currentOri.Row,currentOri.Col,currentOri.Angle,&homMat2DIdentity);
		affine_trans_contour_xld(oRegion,&oRegion,homMat2DIdentity);
	}
	else//只平移（region）
	{
		////2014.6.14 nanjc 日照鼎新横向尺寸、歪脖区域会突然偏离到右侧，而其他类型区域正常，未找到具体原因，修改该平移区域方法，具体是否此处原因需测试
		//HTuple homMat2DTrans;
		//hom_mat2d_identity(&homMat2DTrans);
		//hom_mat2d_translate(homMat2DTrans,currentOri.Row-modelOri.Row,
		//	currentOri.Col-modelOri.Col,&homMat2DTrans);
		//affine_trans_region(oRegion,&oRegion,homMat2DTrans,"false");

		//move_region(oRegion,&oRegion,currentOri.Row-modelOri.Row,
		//	currentOri.Col-modelOri.Col);

		//2014.9.12 区域变换时，若靠边会丢失变小，导致轮廓尺寸区域连续误报，均修改为轮廓
		HTuple homMat2DTrans;
		hom_mat2d_identity(&homMat2DTrans);
		switch(iMethod)//不旋转时有效
		{
		case 0://水平+竖直
			hom_mat2d_translate(homMat2DTrans,currentOri.Row-modelOri.Row,
				currentOri.Col-modelOri.Col,&homMat2DTrans);
			break;
		case 1://水平
			hom_mat2d_translate(homMat2DTrans,0,
				currentOri.Col-modelOri.Col,&homMat2DTrans);
			break;
		case 2://竖直
			hom_mat2d_translate(homMat2DTrans,currentOri.Row-modelOri.Row,
				0,&homMat2DTrans);
			break;
		default:
			hom_mat2d_translate(homMat2DTrans,currentOri.Row-modelOri.Row,
				currentOri.Col-modelOri.Col,&homMat2DTrans);
			break;
		}
		affine_trans_contour_xld(oRegion,&oRegion,homMat2DTrans);
	}
}
void CCheck::affineCircle(Hobject &oRegion)
{
	if (currentOri.Row == 0 && currentOri.Col ==0)
	{
		return;//没有定位
	}
	double dRaidus;
	smallest_circle(oRegion,NULL,NULL,&dRaidus);
	gen_circle(&oRegion,currentOri.Row,currentOri.Col,dRaidus);
}
//*功能：对所有形状变量进行仿射变换
void CCheck::affineAllShape()
{
	int i;
	for (i=0;i<vItemFlow.size();++i)
	{
		switch(vItemFlow[i])
		{
		case ITEM_SIDEWALL_LOCATE://瓶身应力定位：变换瓶子轮廓
			{
				s_pSideLoc pSideLoc = vModelParas[i].value<s_pSideLoc>();
				s_oSideLoc oSideLoc = vModelShapes[i].value<s_oSideLoc>();
				//2013.9.25 nanjc 综合定位
				if(pSideLoc.bStress && pSideLoc.nMethodIdx == 3)
				{
					if (currentOri.Row != 0 || currentOri.Col !=0)
					{
						HTuple homMat2DIdentity;
						hom_mat2d_identity(&homMat2DIdentity);
						vector_angle_to_rigid (oSideLoc.drow1, oSideLoc.dcol1, oSideLoc.dphi1,
							oSideLoc.drow2, oSideLoc.dcol2, oSideLoc.dphi2, &homMat2DIdentity);
						affine_trans_contour_xld(m_normalbotXld,&m_pressbotXld,homMat2DIdentity);
					}
				}
			}
			break;
		//case ITEM_BASE_LOCATE:
		//    {
		//		s_pBaseLoc pBaLoc = vModelParas[i].value<s_pSideLoc>();
		//		s_oBaseLoc oBaLoc = vModelShapes[i].value<s_oSideLoc>();
		//		//方形定位
		//		if(pBaLoc.nMethodIdx == 9)
		//		{
		//			if (currentOri.Row != 0 || currentOri.Col !=0)
		//			{
		//				HTuple homMat2DIdentity;
		//				hom_mat2d_identity(&homMat2DIdentity);
		//				vector_angle_to_rigid (oSideLoc.drow1, oSideLoc.dcol1, oSideLoc.dphi1,
		//					oSideLoc.drow2, oSideLoc.dcol2, oSideLoc.dphi2, &homMat2DIdentity);
		//				affine_trans_contour_xld(m_normalbotXld,&m_pressbotXld,homMat2DIdentity);
		////		
		////vector_angle_to_rigid(modelOri.Row,modelOri.Col,modelOri.Angle,
		////	currentOri.Row,currentOri.Col,currentOri.Angle,&homMat2DIdentity);
		////affine_trans_contour_xld(oRegion,&oRegion,homMat2DIdentity);
		//			}
		//		}
		//	}
			break;
		case ITEM_HORI_SIZE:
			{
				s_oHoriSize oHoriSize = vModelShapes[i].value<s_oHoriSize>();
				affineRegion(oHoriSize.oSizeRect);
				vModelShapes[i].setValue(oHoriSize);
			}
			break;
		case ITEM_VERT_SIZE:
			{
				s_oVertSize oVertSize = vModelShapes[i].value<s_oVertSize>();
				affineRegion(oVertSize.oSizeRect);
				vModelShapes[i].setValue(oVertSize);
			}
			break;
		case ITEM_FULL_HEIGHT:
			{
				s_oFullHeight oFullHeight = vModelShapes[i].value<s_oFullHeight>();
				affineRegion(oFullHeight.oSizeRect,false,1);//只水平移动
				vModelShapes[i].setValue(oFullHeight);
			}
			break;
		case ITEM_BENT_NECK:
			{
				s_oBentNeck oBentNeck = vModelShapes[i].value<s_oBentNeck>();
				affineRegion(oBentNeck.oFinRect);
				vModelShapes[i].setValue(oBentNeck);
			}
			break;
		case ITEM_SGENNERAL_REGION:
			{
				s_oSGenReg oSGenReg = vModelShapes[i].value<s_oSGenReg>();
				affineRegion(oSGenReg.oCheckRegion,true);
				affineRegion(oSGenReg.oCheckRegion_Rect,true);
				vModelShapes[i].setValue(oSGenReg);
			}
			break;
		case ITEM_DISTURB_REGION:
			{
				s_pDistReg pDistReg = vModelParas[i].value<s_pDistReg>();
				if (pDistReg.nRegType != 0 || pDistReg.bIsMove)
				{
					s_oDistReg oDistReg = vModelShapes[i].value<s_oDistReg>();
					affineRegion(oDistReg.oDisturbReg,true);
					affineRegion(oDistReg.oDisturbReg_Rect,true);
					vModelShapes[i].setValue(oDistReg);
				}
			}
			break;
		case ITEM_SSIDEFINISH_REGION:
			{			
				s_oSSideFReg oSSideFReg = vModelShapes[i].value<s_oSSideFReg>();
				affineRegion(oSSideFReg.oCheckRegion,true);
				affineRegion(oSSideFReg.oCheckRegion_Rect,true);
				vModelShapes[i].setValue(oSSideFReg);
			}
			break;
		case ITEM_SINFINISH_REGION:
			{
				s_oSInFReg oSInFReg = vModelShapes[i].value<s_oSInFReg>();
				affineRegion(oSInFReg.oCheckRegion,true);
				vModelShapes[i].setValue(oSInFReg);
			}
			break;
		case ITEM_SSCREWFINISH_REGION:
			{
				s_oSScrewFReg oSScrewFReg = vModelShapes[i].value<s_oSScrewFReg>();
				affineRegion(oSScrewFReg.oCheckRegion,true);
				affineRegion(oSScrewFReg.oCheckRegion_Rect,true);
				vModelShapes[i].setValue(oSScrewFReg);
			}
			break;
		case ITEM_FRLINNER_REGION:
			{
				s_oFRLInReg oFRLInReg = vModelShapes[i].value<s_oFRLInReg>();
				affineCircle(oFRLInReg.oInCircle);
				affineCircle(oFRLInReg.oOutCircle);
				vModelShapes[i].setValue(oFRLInReg);
			}
			break;
		case ITEM_FRLMIDDLE_REGION:
			{
				s_oFRLMidReg oFRLMidReg = vModelShapes[i].value<s_oFRLMidReg>();
				affineCircle(oFRLMidReg.oInCircle);
				affineCircle(oFRLMidReg.oOutCircle);
				vModelShapes[i].setValue(oFRLMidReg);
			}
			break;
		case ITEM_FRLOUTER_REGION:
			{
				s_oFRLOutReg oFRLOutReg = vModelShapes[i].value<s_oFRLOutReg>();
				affineCircle(oFRLOutReg.oInCircle);
				affineCircle(oFRLOutReg.oOutCircle);
				vModelShapes[i].setValue(oFRLOutReg);
			}
			break;
		case ITEM_FBLINNER_REGION:
			{
				s_oFBLInReg oFBLInReg = vModelShapes[i].value<s_oFBLInReg>();
				affineCircle(oFBLInReg.oInCircle);
				affineCircle(oFBLInReg.oOutCircle);
				//oFBLInReg.oRect不需要仿射
				vModelShapes[i].setValue(oFBLInReg);
			}
			break;
		case ITEM_FBLMIDDLE_REGION:
			{
				s_oFBLMidReg oFBLMidReg = vModelShapes[i].value<s_oFBLMidReg>();
				affineCircle(oFBLMidReg.oInCircle);
				affineCircle(oFBLMidReg.oOutCircle);
				vModelShapes[i].setValue(oFBLMidReg);
			}
			break;
		case ITEM_BINNER_REGION:
			{
				s_oBInReg oBInReg = vModelShapes[i].value<s_oBInReg>();
				s_pBInReg pBInReg = vModelShapes[i].value<s_pBInReg>();
				s_pBaseLoc pBaLoc = vModelShapes[i].value<s_pBaseLoc>();
				affineCircle(oBInReg.oInCircle);
				affineCircle(oBInReg.oOutCircle);
				affineRegion(oBInReg.oTriBase,false);
				affineRegion(oBInReg.oRectBase,true);  //rotate result must saved in vModelShape,if not will fail save
				vModelShapes[i].setValue(oBInReg);

			}
			break;
		case ITEM_BMIDDLE_REGION:
			{
				s_oBMidReg oBMidReg = vModelShapes[i].value<s_oBMidReg>();
				affineCircle(oBMidReg.oInCircle);
				affineCircle(oBMidReg.oOutCircle);
				vModelShapes[i].setValue(oBMidReg);
			}
			break;
		case ITEM_BOUTER_REGION:
			{
				s_oBOutReg oBOutReg = vModelShapes[i].value<s_oBOutReg>();
				affineCircle(oBOutReg.oInCircle);
				affineCircle(oBOutReg.oOutCircle);
				vModelShapes[i].setValue(oBOutReg);
			}
			break;
		case ITEM_SSIDEWALL_REGION:
			{
				s_oSSideReg oSSideReg = vModelShapes[i].value<s_oSSideReg>();
				affineRegion(oSSideReg.oCheckRegion,true);
				affineRegion(oSSideReg.oCheckRegion_Rect,true);
				vModelShapes[i].setValue(oSSideReg);
			}
			break;
		case ITEM_NECK_CONTOUR:
			{
				s_oNeckCont oNeckCont = vModelShapes[i].value<s_oNeckCont>();
				affineRegion(oNeckCont.oCheckRegion);
				vModelShapes[i].setValue(oNeckCont);
			}
			break;
		case ITEM_BODY_CONTOUR:
			{
				s_oBodyCont oBodyCont = vModelShapes[i].value<s_oBodyCont>();
				affineRegion(oBodyCont.oCheckRegion);
				vModelShapes[i].setValue(oBodyCont);
			}
			break;
		case ITEM_SBRISPOT_REGION:
			{
				s_oSBriSpotReg oSBriSpotReg = vModelShapes[i].value<s_oSBriSpotReg>();
				affineRegion(oSBriSpotReg.oCheckRegion,true);
				affineRegion(oSBriSpotReg.oCheckRegion_Rect,true);
				vModelShapes[i].setValue(oSBriSpotReg);
			}
			break;
		case ITEM_BALL_REGION:
			{
				s_oBAllReg oBAllReg = vModelShapes[i].value<s_oBAllReg>();
				affineCircle(oBAllReg.oInCircle);
				affineCircle(oBAllReg.oOutCircle);
				vModelShapes[i].setValue(oBAllReg);
			}
			break;	
		case ITEM_CIRCLE_SIZE:
			{
				s_oCirSize oCirSize = vModelShapes[i].value<s_oCirSize>();
				affineCircle(oCirSize.oCircle);
				vModelShapes[i].setValue(oCirSize);
			}
			break;
		case ITEM_SBASE_REGION:
			{
				s_oSBaseReg oSBaseReg = vModelShapes[i].value<s_oSBaseReg>();
				affineCircle(oSBaseReg.oInCircle);
				affineCircle(oSBaseReg.oOutCircle);
				vModelShapes[i].setValue(oSBaseReg);
			}
			break;
		case ITEM_SBASECONVEX_REGION:
			{
				s_oSBaseConvexReg oSBaseConvexReg = vModelShapes[i].value<s_oSBaseConvexReg>();
				affineRegion(oSBaseConvexReg.oCheckRegion,true);
				vModelShapes[i].setValue(oSBaseConvexReg);
			}
			break;
		default:
			break;
		}
	}
}
//*功能：提取排除缺陷
void CCheck::ExtExcludeDefect(RtnInfo &rtnInfo, Hobject &regBig,Hobject &regSmall,
	int nDefectCause,int nDefectType,QString strName,bool bModify/*=true*/)
{
	if (m_bExtExcludeDefect)
	{
		Hlong nNum;
		ExcludeInfo eldInfo;
		Hobject regConBig,regConSmall;
		Hobject regDiff,regSel;
		HTuple tupArea,tupIndice;
		int i;
		connection(regBig,&regConBig);
		connection(regSmall,&regConSmall);
		difference(regConBig,regConSmall,&regDiff);
		//面积太小的不提取
		select_shape(regDiff,&eldInfo.regExclude,"area","and",10,99999999);
		count_obj(eldInfo.regExclude,&nNum);
		if (nNum<1)
		{
			return;
		}
		int icount = rtnInfo.vExcludeInfo.count();
		if (icount>0)
		{
			QList<ExcludeInfo>::iterator iter = rtnInfo.vExcludeInfo.begin();
			//同一检测项中，被已提取的【排除缺陷】完全包含的，仅显示大缺陷
			for (i = 0;i<icount;i++)
			{
				difference(eldInfo.regExclude,iter->regExclude,&regDiff);
				count_obj(regDiff,&nNum);
				if (nNum<1)
				{
					iter++;
					continue;
				}
				tupArea = HTuple();
				area_center (regDiff, &tupArea, NULL, NULL);
				tuple_sgn(tupArea,&tupIndice);
				tuple_find (tupIndice, 1, &tupIndice);
				if (tupIndice[0].I()<0)
				{
					//此处由于当前提取的缺陷已被完全包含，所以直接返回
					return;
				}
				select_obj (eldInfo.regExclude,&(eldInfo.regExclude),tupIndice+1);

				//完全相同的区域已经从eldInfo.regExclude中排除，所以不会被漏掉
				difference(iter->regExclude,eldInfo.regExclude,&regDiff);
				count_obj(regDiff,&nNum);
				if (nNum<1)
				{
					iter++;
					continue;
				}
				tupArea = HTuple();
				area_center (regDiff, &tupArea, NULL, NULL);
				tuple_sgn(tupArea,&tupIndice);
				tuple_find (tupIndice, 1, &tupIndice);
				if (tupIndice[0].I()<0)
				{
					//之前提取的缺陷被当前缺陷完全包含，则将之前缺陷置空
					gen_empty_obj(&(iter->regExclude));
					iter++;
					continue;
				}
				select_obj (iter->regExclude,&(iter->regExclude),tupIndice+1);
				iter++;
			}
		}
		count_obj(eldInfo.regExclude,&nNum);
		if (nNum>0)
		{
			eldInfo.nDefectCause = nDefectCause;
			eldInfo.nDefectType = nDefectType;
			eldInfo.strDetName = strName;
			eldInfo.bModify = bModify;
			rtnInfo.vExcludeInfo.push_back(eldInfo);
		}
	}
}
//*提取排除缺陷（极坐标）
void CCheck::ExtExcludePolarDefect(RtnInfo &rtnInfo, Hobject &regBig,Hobject &regSmall,double dInR,double dOutR,
	int nDefectCause,int nDefectType,QString strName,bool bModify/*=true*/)
{
	if (m_bExtExcludeDefect)
	{
		double dOriRow = currentOri.Row;
		double dOriCol = currentOri.Col;
		Hobject regBigPolar,regSmallPolar;
		polar_trans_region_inv(regBig,&regBigPolar,dOriRow,dOriCol,0,2*PI,dInR,dOutR,
			m_nWidth,dOutR-dInR,m_nWidth,m_nHeight,"nearest_neighbor");
		polar_trans_region_inv(regSmall,&regSmallPolar,dOriRow,dOriCol,0,2*PI,dInR,dOutR,
			m_nWidth,dOutR-dInR,m_nWidth,m_nHeight,"nearest_neighbor");
		ExtExcludeDefect(rtnInfo,regBigPolar,regSmallPolar,nDefectCause,nDefectType,strName,bModify);
	}
}
//*提取排除缺陷（仿射变换）
void CCheck::ExtExcludeAffinedDefect(RtnInfo &rtnInfo, Hobject &regBig,Hobject &regSmall,int noffSet,HTuple homMat,
	int nDefectCause,int nDefectType,QString strName,bool bModify/*=true*/)
{
	if (m_bExtExcludeDefect)
	{
		Hlong nNum;
		ExcludeInfo eldInfo;
		Hobject regConBig,regConSmall;
		Hobject regDiff,regSel;
		HTuple tupArea,tupIndice;
		int i;
		connection(regBig,&regConBig);
		connection(regSmall,&regConSmall);
		difference(regConBig,regConSmall,&regDiff);
		//移动+变换
		move_region(regDiff, &regDiff, 0, noffSet);
		affine_trans_region(regDiff, &regDiff, homMat, "false");	
		//面积太小的不提取
		select_shape(regDiff,&eldInfo.regExclude,"area","and",10,99999999);
		count_obj(eldInfo.regExclude,&nNum);
		if (nNum<1)
		{
			return;
		}
		int icount = rtnInfo.vExcludeInfo.count();
		if (icount>0)
		{
			QList<ExcludeInfo>::iterator iter = rtnInfo.vExcludeInfo.begin();
			//同一检测项中，被已提取的【排除缺陷】完全包含的，仅显示大缺陷
			for (i = 0;i<icount;i++)
			{
				difference(eldInfo.regExclude,iter->regExclude,&regDiff);
				count_obj(regDiff,&nNum);
				if (nNum<1)
				{
					iter++;
					continue;
				}
				tupArea = HTuple();
				area_center (regDiff, &tupArea, NULL, NULL);
				tuple_sgn(tupArea,&tupIndice);
				tuple_find (tupIndice, 1, &tupIndice);
				if (tupIndice[0].I()<0)
				{
					//此处由于当前提取的缺陷已被完全包含，所以直接返回
					return;
				}
				select_obj (eldInfo.regExclude,&(eldInfo.regExclude),tupIndice+1);

				//完全相同的区域已经从eldInfo.regExclude中排除，所以不会被漏掉
				difference(iter->regExclude,eldInfo.regExclude,&regDiff);
				count_obj(regDiff,&nNum);
				if (nNum<1)
				{
					iter++;
					continue;
				}
				tupArea = HTuple();
				area_center (regDiff, &tupArea, NULL, NULL);
				tuple_sgn(tupArea,&tupIndice);
				tuple_find (tupIndice, 1, &tupIndice);
				if (tupIndice[0].I()<0)
				{
					//之前提取的缺陷被当前缺陷完全包含，则将之前缺陷置空
					gen_empty_obj(&(iter->regExclude));
					iter++;
					continue;
				}
				select_obj (iter->regExclude,&(iter->regExclude),tupIndice+1);
				iter++;
			}
		}

		count_obj(eldInfo.regExclude,&nNum);
		if (nNum>0)
		{
			eldInfo.nDefectCause = nDefectCause;
			eldInfo.nDefectType = nDefectType;
			eldInfo.strDetName = strName;
			eldInfo.bModify = bModify;
			rtnInfo.vExcludeInfo.push_back(eldInfo);
		}
	}
}

//*过滤排除缺陷
void CCheck::FilterExcludeDefect(RtnInfo &rtnInfo)
{
	if (!m_bExtExcludeDefect || rtnInfo.nType<1)
	{
		return;
	}
	Hobject regCon,regInter;
	Hlong nNum;
	HTuple tupArea,tupIndice;
	int i;
	int icount = m_vExcludeInfo.count();
	if (icount<1)
	{
		return;
	}
	QList<ExcludeInfo>::iterator iter = m_vExcludeInfo.begin();
	for (i = 0;i<icount;i++)
	{  
		connection (iter->regExclude, &regCon);
		count_obj(regCon,&nNum);
		if (nNum<1)
		{
			iter = m_vExcludeInfo.erase(iter);
			continue;
		}
		//过滤掉排除区域中与缺陷区域有交集的区域
		intersection (regCon, rtnInfo.regError,&regInter);
		count_obj(regInter,&nNum);
		if (nNum<1)
		{
			iter++;
			continue;
		}
		tupArea = HTuple();
		area_center (regInter, &tupArea, NULL, NULL);
		tuple_sgn(tupArea,&tupIndice);
		tuple_find (tupIndice, 0, &tupIndice);
		if (tupIndice[0].I()<0)
		{
			iter = m_vExcludeInfo.erase(iter);
			continue;
		}
		select_obj (regCon,&(iter->regExclude),tupIndice+1);
		iter++;
	}
}
//*功能：计算当前原点
RtnInfo CCheck::fnFindCurrentPos(Hobject &bottleXld, bool bInit)
{
	RtnInfo rtnInfo;
	gen_empty_obj(&bottleXld);
	for (int i=0;i<vItemFlow.size();++i)
	{
		switch(vItemFlow[i])
		{
		case ITEM_SIDEWALL_LOCATE:
			rtnInfo = fnFindPosSidewall(m_ImageSrc,vModelParas[i],vModelShapes[i],bottleXld,bInit);			
			break;
		case ITEM_FINISH_LOCATE:
			rtnInfo = fnFindPosFinish(m_ImageSrc,vModelParas[i],vModelShapes[i]);
			break;
		case ITEM_BASE_LOCATE:
			rtnInfo = fnFindPosBase(m_ImageSrc,vModelParas[i],vModelShapes[i]);
			break;			
		default:
			break;
		}
		if (rtnInfo.nType>0)
		{
			rtnInfo.nItem = i;//用于通知检测项风格变化
			break;
		}			
	}
	return rtnInfo;
}
//*功能：瓶身定位
RtnInfo CCheck::fnFindPosSidewall(Hobject &imgSrc,QVariant &para,QVariant &shape,Hobject &bottleXld,bool bInit)
{
	RtnInfo rtnInfo;
	s_pSideLoc pSideLoc = para.value<s_pSideLoc>();
	s_oSideLoc oSideLoc = shape.value<s_oSideLoc>();

	int nType = pSideLoc.nDirect;//查找方向
	bool bMean = pSideLoc.bStress;//应力需要平滑
	bool bFindPointSubPix = pSideLoc.bFindPointSubPix; //第三条线查找边界点是否采用亚像素法

	Hobject bottleReg;
	int iBottleRegWidth,iBottleRegHeight;
	int iRow1,iCol1,iRow2,iCol2;
	//double Phi;
	int ioffset = 30;
	gen_empty_obj(&bottleXld);
	switch(pSideLoc.nMethodIdx)
	{
	case 0://平移旋转
		{
			rtnInfo = findPosThreeLine(imgSrc,oSideLoc.oFirstLine,oSideLoc.oSecondLine,oSideLoc.oThirdLine,
				oldSideOri,oSideLoc.ori,pSideLoc.nEdge,pSideLoc.nFloatRange/100.f,nType,bMean,bFindPointSubPix);
			//2013.9.24 nanjc 瓶子轮廓区域
			if (rtnInfo.nType==0 && !pSideLoc.bStress)
			{
				iCol1 = min(oSideLoc.ori.nCol11,oSideLoc.ori.nCol21);
				iCol2 = max(oSideLoc.ori.nCol12,oSideLoc.ori.nCol22);
				iRow1 = oSideLoc.ori.Row;
				iRow2 = max((oSideLoc.ori.nRow11+oSideLoc.ori.nRow12)/2,(oSideLoc.ori.nRow21+oSideLoc.ori.nRow22)/2);
				iBottleRegWidth = iCol2 - iCol1;
				iBottleRegHeight = iRow2-iRow1;
				if (iBottleRegWidth>0 && iBottleRegHeight>0)
				{
					gen_rectangle1(&bottleReg,iRow1-ioffset,iCol1-ioffset,iRow2+ioffset,iCol2+ioffset);
					getBottleXld(imgSrc,bottleReg,bottleXld,iBottleRegWidth,iBottleRegHeight,3);
				}			
			}
		}
		break;
	case 1://横向平移
		{
			int nLeftRight = oSideLoc.nLeftRight;
			rtnInfo = findXTwoLine(imgSrc,oSideLoc.oFirstLine,oSideLoc.oSecondLine,oSideLoc.ori,
				pSideLoc.nEdge,nLeftRight,nType,bMean);
			//2013.9.24 nanjc 瓶子轮廓区域
			if (rtnInfo.nType==0 && !pSideLoc.bStress)
			{
				ioffset = 100;
				iBottleRegWidth = abs(oSideLoc.ori.nCol21-oSideLoc.ori.nCol11);
				iBottleRegHeight = abs(oSideLoc.ori.nRow21-oSideLoc.ori.nRow11);
				if (iBottleRegHeight>0)
				{
					iRow1 = m_nHeight/2-20;
					if (nLeftRight == 1)
					{
						iCol1 = abs(oSideLoc.ori.nCol11+oSideLoc.ori.nCol21)/2 + ioffset*3/4;
					}
					else
					{
						iCol1 = abs(oSideLoc.ori.nCol11+oSideLoc.ori.nCol21)/2 - ioffset*3/4;
					}
					gen_rectangle2(&bottleReg,iRow1,iCol1,oSideLoc.ori.Angle,m_nHeight/2,ioffset);
					getBottleXld(imgSrc,bottleReg,bottleXld,iBottleRegWidth,iBottleRegHeight,nLeftRight);
				}			
			}
		}
		break;
	case 2://单侧平移旋转
		{
			int nLeftRight = oSideLoc.nLeftRight;
			// 修改了定位线之后需要重新计算距离
			if (bInit)
			{
				oSideLoc.ori.fDist3=CalcDistPt3ToOri(imgSrc, oSideLoc.oFirstLine,oSideLoc.oSecondLine,oSideLoc.oThirdLine,
					pSideLoc.nEdge,nLeftRight,nType,bMean);					
			}
			rtnInfo = findPosThreePoints(imgSrc,oSideLoc.oFirstLine,oSideLoc.oSecondLine,oSideLoc.oThirdLine,
				oSideLoc.ori,pSideLoc.nEdge,nLeftRight,nType,bMean);
			//2013.9.24 nanjc 瓶子轮廓区域
			if (rtnInfo.nType==0 && !pSideLoc.bStress)
			{
				iCol1 = min(min(oSideLoc.ori.nCol11,oSideLoc.ori.nCol21),oSideLoc.ori.Col);
				iCol2 = max(max(oSideLoc.ori.nCol12,oSideLoc.ori.nCol22),oSideLoc.ori.Col);
				iRow1 = oSideLoc.ori.Row;
				iRow2 = max((oSideLoc.ori.nRow11+oSideLoc.ori.nRow12)/2,(oSideLoc.ori.nRow21+oSideLoc.ori.nRow22)/2);
				iBottleRegWidth = iCol2 - iCol1;
				iBottleRegHeight = iRow2-iRow1;
				if (iBottleRegWidth>0 && iBottleRegHeight>0)
				{
					ioffset = 30;
					gen_rectangle1(&bottleReg,iRow1-ioffset,iCol1-ioffset,iRow2+ioffset,iCol2+ioffset);
					getBottleXld(imgSrc,bottleReg,bottleXld,iBottleRegWidth,iBottleRegHeight,nLeftRight);
				}			
			}
		}		
		break;
	case 3://2013.9.22 nanjc 正常图像定位应力图像:新原点=传递进来的正常图像原点+位移
		{
			if (pSideLoc.bStress)
			{
				if (normalOri.Row==0 || normalOri.Col==0)
				{
					rtnInfo.nType = ERROR_LOCATEFAIL;
					rtnInfo.strEx = QObject::tr("Normal image locates failure, stress image can not locate!");
				}
				else
				{
					oSideLoc.ori.Row = normalOri.Row + oSideLoc.drow2 - oSideLoc.drow1;
					oSideLoc.ori.Col = normalOri.Col + oSideLoc.dcol2 - oSideLoc.dcol1;
					oSideLoc.ori.Angle = normalOri.Angle + oSideLoc.dphi2 - oSideLoc.dphi1;
					//HTuple homMat2DIdentity;
					//hom_mat2d_identity(&homMat2DIdentity);
					//vector_angle_to_rigid (oSideLoc.nrow1, oSideLoc.ncol1, oSideLoc.phi1,
					//	oSideLoc.nrow2, oSideLoc.ncol2, oSideLoc.phi2, &homMat2DIdentity)
					//affine_trans_contour_xld(oRegion,&oRegion,homMat2DIdentity);
				}
			}		
		}
		break;
	case 4://横向纵向平移
		{
			rtnInfo = findXYTwoLine(imgSrc,oSideLoc.oFirstLine,oSideLoc.oThirdLine,oldSideOri,oSideLoc.ori,
				pSideLoc.nEdge,pSideLoc.nFloatRange/100.f,nType,bMean,bFindPointSubPix);
			//2013.9.24 nanjc 瓶子轮廓区域
			if (rtnInfo.nType==0 && !pSideLoc.bStress)
			{
				iCol1 = min(oSideLoc.ori.nCol11,oSideLoc.ori.nCol21);
				iCol2 = max(oSideLoc.ori.nCol12,oSideLoc.ori.nCol22);
				iRow1 = oSideLoc.ori.Row;
				iRow2 = max((oSideLoc.ori.nRow11+oSideLoc.ori.nRow12)/2,(oSideLoc.ori.nRow21+oSideLoc.ori.nRow22)/2);
				iBottleRegWidth = iCol2 - iCol1;
				iBottleRegHeight = iRow2-iRow1;
				if (iBottleRegWidth>0 && iBottleRegHeight>0)
				{
					ioffset = 30;
					gen_rectangle1(&bottleReg,iRow1-ioffset,iCol1-ioffset,iRow2+ioffset,iCol2+ioffset);
					getBottleXld(imgSrc,bottleReg,bottleXld,iBottleRegWidth,iBottleRegHeight,3);
				}			
			}
		}
		break;
	case 5://中心横向平移
		{
			rtnInfo = findXTwoLineConBottle(imgSrc,oSideLoc.oFirstLine,oSideLoc.oSecondLine,oldSideOri,oSideLoc.ori,
				pSideLoc.nEdge,pSideLoc.nFloatRange/100.f,oSideLoc.nLeftRight,nType,bMean);
			//2013.9.24 nanjc 瓶子轮廓区域
			if (rtnInfo.nType==0 && !pSideLoc.bStress)
			{
				ioffset = 100;
				iBottleRegWidth = abs(oSideLoc.ori.nCol21-oSideLoc.ori.nCol11);
				iBottleRegHeight = abs(oSideLoc.ori.nRow21-oSideLoc.ori.nRow11);
				if (iBottleRegHeight>0)
				{
					iRow1 = m_nHeight/2-20;
					iCol1 = abs(oSideLoc.ori.nCol11+oSideLoc.ori.nCol21)/2 + ioffset*3/4;
					gen_rectangle2(&bottleReg,iRow1,iCol1,oSideLoc.ori.Angle,m_nHeight/2,ioffset);
					getBottleXld(imgSrc,bottleReg,bottleXld,iBottleRegWidth,iBottleRegHeight,1);
				}	
			}
		}
		break;
	case 6://平移旋转（应力）---2018.1.应力图像自己定位，不利用正常图像的定位信息
		{		
		    if (pSideLoc.bStress)
		    {		    
				rtnInfo = findPosThreeLineStress(imgSrc,oSideLoc.oFirstLine,oSideLoc.oSecondLine,oSideLoc.oThirdLine,
				           oldSideOri,oSideLoc.ori,pSideLoc.nEdge,pSideLoc.nFloatRange/100.f,nType,bMean);
			}
		}
		break;
	default:
		break;
	}

	shape.setValue(oSideLoc);
	if (rtnInfo.nType>0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	}
	else
	{
		//更新当前原点
		currentOri.Row = oSideLoc.ori.Row;
		currentOri.Col = oSideLoc.ori.Col;
		currentOri.Angle = oSideLoc.ori.Angle;
	}
	return rtnInfo;
}
int CCheck::FindEdgePointSubPix(const Hobject &Image, const Hobject &LineSeg, 
	HTuple *RowPt, HTuple *ColPt, int nEdge, int nDirect,int nType)
{
	Hlong nNum = 0;
	Hlong nCount = 0;
	HTuple Rows, Cols;
	double RowMean, ColMean;

	// 清空目的数组
	*RowPt = HTuple();
	*ColPt = HTuple();

	Hobject RegGen, ImgReduce, Edges, Xld1, Xld2;
	dilation_circle(LineSeg, &RegGen, 3.5);
	reduce_domain(Image, RegGen, &ImgReduce);
	edges_sub_pix(ImgReduce, &Edges, "canny", 1, nEdge, (2*nEdge));
	select_contours_xld(Edges, &Edges, "contour_length", 3, 99999, 0, 0);
	select_shape_xld(Edges, &Edges, "width","and", 5, 99999);
	count_obj(Edges, &nNum);

	if (nNum == 0)
	{
		nCount = 0;
	}
	else if (nNum > 1)
	{
		switch(nDirect)
		{
		case L2R:
			sort_contours_xld(Edges, &Edges, "upper_left", "true", "column");
			break;
		case R2L:
			sort_contours_xld(Edges, &Edges, "upper_left", "false", "column");
			break;
		case T2B:
			sort_contours_xld(Edges, &Edges, "upper_left", "true", "row");
			break;
		case B2T:
			sort_contours_xld(Edges, &Edges, "upper_left", "false", "row");
			break;
		default:
			break;
		}
		//选择最上面的第一个点
		select_obj(Edges, &Xld1, 1);
		get_contour_xld(Xld1, &Rows, &Cols);
		tuple_mean(Rows, &RowMean);
		tuple_mean(Cols, &ColMean);
		(*RowPt)[0] = RowMean;
		(*ColPt)[0] = ColMean;

		////选择最右面的第二个点
		//select_obj(Edges, &Xld2, nNum);
		//get_contour_xld(Xld2, &Rows, &Cols);
		//tuple_mean(Rows, &RowMean);
		//tuple_mean(Cols, &ColMean);
		//(*RowPt)[1] = RowMean;
		//(*ColPt)[1] = ColMean;

		nCount = 1;
	}
	else
	{
		get_contour_xld(Edges, &Rows, &Cols);
		tuple_mean(Rows, &RowMean);
		tuple_mean(Cols, &ColMean);
		(*RowPt)[0] = RowMean;
		(*ColPt)[0] = ColMean;

		nCount = 1;
	}

	return nCount;
}


//*功能：获取一个边缘点，返回边缘点的个数
int CCheck::findEdgePointSingle(const Hobject &Image, const Hobject &LineSeg, 
	HTuple *RowPt, HTuple *ColPt, int nEdge/*= 10*/, int nDirect/* = L2R*/, int nType/* = 0*/, 
	BOOL meanImg/* = FALSE*/)
{
	// 清空目的数组
	*RowPt = HTuple();
	*ColPt = HTuple();

	Hobject ImageReduce, ImageGauss, NewLine;
	HTuple Row, Col, GrayValue;
	Hlong nGrayDiff1, nGrayDiff2, nGrayDiff3, nCount, nLength, diff;
	Hlong nImgWidth, nImgHeight;
	double dPhi;

	// 边界点的数目
	nCount = 0;

	get_region_points(LineSeg, &Row, &Col);
	nLength = Col.Num();
	if (nLength < 4 )
	{
		return nCount;
	}

	// 对行列坐标数组排序
	diff = Col[0] - Col[nLength - 1];
	orientation_region(LineSeg, &dPhi);
	dPhi = fabs(dPhi);	
	switch(nDirect)
	{
	case L2R:
		if (diff > 0)
		{
			Row = Row.Inverse();
			Col = Col.Sort();
		}
		break;
	case R2L:
		if (diff < 0)
		{
			Row = Row.Inverse();
			Col = Col.Inverse();
		}
		else if (diff > 0 && dPhi < PI/4)
		{
			Col = Col.Sort();
			Col = Col.Inverse();
		}
		break;
	case T2B:
		if (diff > 0 && dPhi < PI/4)
		{
			Col = Col.Sort();
			Col = Col.Inverse();
		}
		break;
	case B2T:
		if (diff < 0)
		{
			Row = Row.Inverse();
			Col = Col.Inverse();
		}
		else
		{
			Row = Row.Inverse();
			Col = Col.Sort();
		}
		break;
	default:
		break;
	}

	// 中值滤波
	Hobject ImgMedian, ImgDomain, RegInter;
	Hlong nNum;
	get_domain(Image, &ImgDomain);
	intersection(LineSeg, ImgDomain, &RegInter);
	connection(RegInter, &RegInter);
	select_shape(RegInter, &RegInter, "area", "and", 1, 99999999);
	count_obj(RegInter, &nNum);
	if (nNum == 0)
	{
		return nCount;
	}
	reduce_domain(Image, LineSeg, &ImgMedian);
	if (meanImg)
	{
		Hobject regDilation,imgMean;
		dilation_circle(LineSeg,&regDilation,3.5);
		reduce_domain(Image,regDilation,&imgMean);
		mean_image(imgMean,&imgMean,3,3);//应力检测需要平滑
		reduce_domain(imgMean,LineSeg,&ImgMedian);
	}
	median_image(ImgMedian, &ImgMedian, "circle", 5, "continued");

	// 获得和坐标点对应的像素灰度值数组
	unsigned char *ptr;
	int i, x, y;

	get_image_pointer1(ImgMedian, (Hlong*)&ptr, NULL, &nImgWidth, &nImgHeight);

	for (i = 0; i < nLength; ++i)
	{
		x = Col[i];
		y = Row[i];
		if(x < nImgWidth && y < nImgHeight)
		{
			GrayValue[i] = ptr[y * nImgWidth + x];
		}
		else
		{
			GrayValue[i] = 0;
		}		
	}

	//  做差分的点从相邻点改为间隔为3的点

	if (nType==0)//灰度由高到低
	{		
		for (i = 0; i < nLength - 5; ++i)
		{
			nGrayDiff1 = GrayValue[i] - GrayValue[i+3];
			nGrayDiff2 = GrayValue[i+1] - GrayValue[i+4];
			nGrayDiff3 = GrayValue[i+2] - GrayValue[i+5];
			if (nGrayDiff1 > nEdge && nGrayDiff2 > nEdge && nGrayDiff3 > nEdge)
			{
				(*RowPt)[nCount] = HTuple(Row[i+3]);
				(*ColPt)[nCount] = HTuple(Col[i+3]);
				++nCount;
				break;
			}
		}
	}
	else if (nType==1)
	{
		for (i = 0; i < nLength - 5; ++i)
		{
			nGrayDiff1 = GrayValue[i+3] - GrayValue[i];
			nGrayDiff2 = GrayValue[i+4] - GrayValue[i+1];
			nGrayDiff3 = GrayValue[i+5] - GrayValue[i+2];
			if (nGrayDiff1 > nEdge && nGrayDiff2 > nEdge && nGrayDiff3 > nEdge)
			{
				(*RowPt)[nCount] = HTuple(Row[i+3]);
				(*ColPt)[nCount] = HTuple(Col[i+3]);
				++nCount;
				break;
			}
		}
	}
	return nCount;
}
//*功能：获取两个边缘点，返回边缘点的个数
int CCheck::findEdgePointDouble(const Hobject &Image, const Hobject &LineSeg, 
	HTuple *RowPt, HTuple *ColPt, int nEdge/*=10*/, int nDirect/*=L2R*/,int nType/*=0*/,BOOL bMean/*=FALSE*/)
{
	// 清空目的数组
	*RowPt = HTuple();
	*ColPt = HTuple();

	Hobject ImageReduce, ImageGauss, NewLine;
	HTuple Row, Col, GrayValue;
	Hlong nGrayDiff1, nGrayDiff2, nGrayDiff3, nCount, nLength, diff;
	Hlong nImgWidth, nImgHeight;
	double dPhi;

	// 边界点的数目
	nCount = 0;

	get_region_points(LineSeg, &Row, &Col);
	nLength = Col.Num();
	if (nLength < 4 )
	{
		return nCount;
	}

	// 对行列坐标数组排序
	diff = Col[0] - Col[nLength - 1];
	orientation_region(LineSeg, &dPhi);
	dPhi = fabs(dPhi);
	switch(nDirect)
	{
	case L2R:
		if (diff > 0)
		{
			Row = Row.Inverse();
			Col = Col.Sort();
		}
		break;
	case R2L:
		if (diff < 0)
		{
			Row = Row.Inverse();
			Col = Col.Inverse();
		}
		else if (diff > 0 && dPhi < PI/4)
		{
			Col = Col.Sort();
			Col = Col.Inverse();
		}
		break;
	case T2B:
		if (diff > 0 && dPhi < PI/4)
		{
			Col = Col.Sort();
			Col = Col.Inverse();
		}
		break;
	case B2T:
		if (diff < 0)
		{
			Row = Row.Inverse();
			Col = Col.Inverse();
		}
		else
		{
			Row = Row.Inverse();
			Col = Col.Sort();
		}
		break;
	default:
		break;
	}

	// 中值滤波,防止光源污点干扰
	Hobject ImgMedian, ImgDomain, RegInter;
	Hlong nNum;
	get_domain(Image, &ImgDomain);
	intersection(LineSeg, ImgDomain, &RegInter);
	connection(RegInter, &RegInter);
	select_shape(RegInter, &RegInter, "area", "and", 1, 99999999);
	count_obj(RegInter, &nNum);
	if (nNum == 0)
	{
		return nCount;
	}
	reduce_domain(Image, LineSeg, &ImgMedian);
	if (bMean)
	{
		Hobject regDilation,imgMean;
		dilation_circle(LineSeg,&regDilation,3.5);
		reduce_domain(Image,regDilation,&imgMean);
		mean_image(imgMean,&imgMean,3,3);//应力检测需要平滑
		reduce_domain(imgMean,LineSeg,&ImgMedian);
	}
	median_image(ImgMedian, &ImgMedian, "circle", 5, "continued");

	// 获得和坐标点对应的像素灰度值数组
	unsigned char *ptr;
	int i, j, x, y;

	get_image_pointer1(ImgMedian, (Hlong*)&ptr, NULL, &nImgWidth, &nImgHeight);

	for (i = 0; i < nLength; ++i)
	{
		x = Col[i];
		y = Row[i];
		if(x < nImgWidth && y < nImgHeight)
		{
			GrayValue[i] = ptr[y * nImgWidth + x];
		}
		else
		{
			GrayValue[i] = 0;
		}		
	}

	//  做差分的点从相邻点改为间隔为2的点

	if (nType==0)//灰度由高到低
	{		
		for (i = 0; i < nLength - 5; ++i)
		{
			nGrayDiff1 = GrayValue[i]   - GrayValue[i+3];
			nGrayDiff2 = GrayValue[i+1] - GrayValue[i+4];
			nGrayDiff3 = GrayValue[i+2] - GrayValue[i+5];
			if (nGrayDiff1 > nEdge && nGrayDiff2 > nEdge && nGrayDiff3 > nEdge)
			{
				(*RowPt)[nCount] = HTuple(Row[i+3]);
				(*ColPt)[nCount] = HTuple(Col[i+3]);
				++nCount;
				break;
			}
		}

		if (nCount > 0)
		{
			for (j = nLength-1; j > i+8; --j)
			{
				nGrayDiff1 = GrayValue[j]   - GrayValue[j-3];
				nGrayDiff2 = GrayValue[j-1] - GrayValue[j-4];
				nGrayDiff3 = GrayValue[j-2] - GrayValue[j-5];
				if (nGrayDiff1 > nEdge && nGrayDiff2 > nEdge && nGrayDiff3 > nEdge)
				{
					(*RowPt)[nCount] = HTuple(Row[j-3]);
					(*ColPt)[nCount] = HTuple(Col[j-3]);

					++nCount;
					break;
				}
			}
		}
	}
	else if (nType==1)
	{
		for (i = 0; i < nLength - 5; ++i)
		{
			nGrayDiff1 = GrayValue[i+3] - GrayValue[i];
			nGrayDiff2 = GrayValue[i+4] - GrayValue[i+1];
			nGrayDiff3 = GrayValue[i+5] - GrayValue[i+2];
			if (nGrayDiff1 > nEdge && nGrayDiff2 > nEdge && nGrayDiff3 > nEdge)
			{
				(*RowPt)[nCount] = HTuple(Row[i+3]);
				(*ColPt)[nCount] = HTuple(Col[i+3]);
				++nCount;
				break;
			}
		}

		if (nCount > 0)
		{
			for (j = nLength-1; j > i+8; --j)
			{
				nGrayDiff1 = GrayValue[j-3] - GrayValue[j];
				nGrayDiff2 = GrayValue[j-4] - GrayValue[j-1];
				nGrayDiff3 = GrayValue[j-5] - GrayValue[j-2];
				if (nGrayDiff1 > nEdge && nGrayDiff2 > nEdge && nGrayDiff3 > nEdge)
				{
					(*RowPt)[nCount] = HTuple(Row[j-3]);
					(*ColPt)[nCount] = HTuple(Col[j-3]);

					++nCount;
					break;
				}
			}
		}
	}
	return nCount;
}
//*功能：基于原点模板的数据判断新原点是否准确，主要防止连瓶、光源脏等
int CCheck::testEdgePointDouble(const Hobject &Image, const Hobject &LineSeg, 
	HTuple &RowPt,HTuple &ColPt, int nEdge, float fRange,
	int FirOrSecLine,SideOrigin &oldOri,int nType/*=0*/,BOOL bMean/*=FALSE*/)
{
	float oldWidth,newWidth;
	float range=fRange;//宽度浮动范围比例	
	int newPtCol,newPtRow;
	int tempColL,tempColR;
	Hlong row1,row2,numTemp;
	Hobject tempLineL,tempLineR,concatLines,newLine;
	HTuple rPtTemp,cPtTemp;

	if (FirOrSecLine ==1)//第一条线
	{
		oldWidth=oldOri.fDist1;		
	}
	else 
	{
		oldWidth=oldOri.fDist2;		
	}
	newWidth=ColPt[1].I()-ColPt[0].I();
	if (fabs(newWidth-oldWidth)/(oldWidth+0.0001)<range)//???
		return 0;
	smallest_rectangle1(LineSeg,&row1,NULL,&row2,NULL);
	//以左点为基准，向右找线
	tempColL=ColPt[0].I()+oldWidth*(1-range/2);
	tempColR=ColPt[0].I()+oldWidth*(1+range/2);
	gen_region_line(&tempLineL,row1-5,tempColL,row2+5,tempColL);
	gen_region_line(&tempLineR,row1-5,tempColR,row2+5,tempColR);
	concat_obj(tempLineL,tempLineR,&concatLines);
	union1(concatLines,&concatLines);
	difference(LineSeg,concatLines,&newLine);
	connection(newLine,&newLine);
	select_shape(newLine,&newLine,"area","and",1,99999999);
	count_obj(newLine,&numTemp);
	if (numTemp!=3)
		return 0;
	sort_region(newLine,&newLine,"upper_left","true","column");
	select_obj(newLine,&newLine,2);
	int nRet=findEdgePointSingle(Image,newLine,&rPtTemp,&cPtTemp,nEdge,R2L,nType,bMean);
	if (nRet==1)
	{			
		newPtCol=cPtTemp[0];	
		newPtRow=rPtTemp[0];
		if (fabs(newPtCol-ColPt[0].I()-oldWidth)/oldWidth<range)
		{
			ColPt[1]=newPtCol;
			RowPt[1]=newPtRow;
			return 0;
		}
	}
	rPtTemp=HTuple();
	cPtTemp=HTuple();
	//以右点为基准，向左找线
	tempColL=ColPt[1].I()-oldWidth*(1+range/2);
	tempColR=ColPt[1].I()-oldWidth*(1-range/2);
	gen_region_line(&tempLineL,row1-5,tempColL,row2+5,tempColL);
	gen_region_line(&tempLineR,row1-5,tempColR,row2+5,tempColR);
	concat_obj(tempLineL,tempLineR,&concatLines);
	union1(concatLines,&concatLines);
	difference(LineSeg,concatLines,&newLine);
	connection(newLine,&newLine);
	select_shape(newLine,&newLine,"area","and",1,99999999);
	count_obj(newLine,&numTemp);
	if (numTemp!=3)
		return 0;
	sort_region(newLine,&newLine,"upper_left","true","column");
	select_obj(newLine,&newLine,2);
	nRet=findEdgePointSingle(Image,newLine,&rPtTemp,&cPtTemp,nEdge,L2R,nType,bMean);
	if (nRet==1)
	{			
		newPtCol=cPtTemp[0];	
		newPtRow=rPtTemp[0];
		if (fabs(ColPt[1].I()-newPtCol-oldWidth)/oldWidth<range)
		{
			ColPt[0]=newPtCol;
			RowPt[0]=newPtRow;
			return 0;
		}
	}
	if (newWidth>oldWidth)
	{
		return 1;//左右大于实际
	}
	else
		return 2;//左右小于实际
}
//*功能：基于三条线计算瓶身原点（平移旋转）
RtnInfo CCheck::findPosThreeLine(const Hobject &Image, const Hobject &FirstLine, const Hobject &SecondLine,	const Hobject &ThirdLine,
	SideOrigin oldOri,SideOrigin &newOri,int nEdge,float fRange,int nType/*=0*/, bool bMean/*=FALSE*/,bool bFindPointSubPix/*=FALSE*/)
{
	RtnInfo rtnInfo;

	Hlong nRowLeft1, nColLeft1, nRowRight1, nColRight1;
	Hlong nRowLeft2, nColLeft2, nRowRight2, nColRight2;
	HTuple Row, Col;
	int testRtn;

	rtnInfo.nType = ERROR_LOCATEFAIL;
	rtnInfo.strEx = QObject::tr("Failure to locate the first line!");

	if(findEdgePointDouble(Image, FirstLine, &Row, &Col, nEdge, L2R, nType,bMean) < 2)
	{
		return rtnInfo;
	}
	if (fRange>0)//自动纠错
	{
		testRtn=testEdgePointDouble(Image,FirstLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean);
		if(testRtn!=0)/*左右点都不正确时,缩放line重定位*/
		{	
			Hobject zoomLine;
			if (testRtn==1)//左右大于实际
			{
				gen_region_line(&zoomLine,Row[0].L(),Col[0].L(),Row[1].L(),Col[1].L());
				Row=HTuple();
				Col=HTuple();
				if (findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean)<2)
				{					
					return rtnInfo;
				}
				if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean)!=0)
				{					
					return rtnInfo;
				}
			} else if (testRtn==2)//左右小于实际
			{
				int MoveDist=30;//左右扩展的长度
				double Phi;
				double rowLeft,colLeft,rowRight,colRight;
				line_orientation(Row[0].D(),Col[0].D(),Row[1].D(),Col[1].D(),&Phi);
				rowLeft=Row[0].D()+MoveDist*sin(Phi);
				colLeft=Col[0].D()-MoveDist*cos(Phi);
				rowRight=Row[1].D()-MoveDist*sin(Phi);
				colRight=Col[1].D()+MoveDist*cos(Phi);
				gen_region_line(&zoomLine,rowLeft,colLeft,rowRight,colRight);
				if (findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean)<2)
				{
					return rtnInfo;
				}
				if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean)!=0)
				{
					return rtnInfo;
				}
			}
		}
	}

	nColLeft1 = Col[0];
	nColRight1 = Col[1];
	nRowLeft1 = Row[0];
	nRowRight1 = Row[1];

	newOri.nRow11 = Row[0];
	newOri.nCol11 = Col[0];
	newOri.nRow12 = Row[1];
	newOri.nCol12 = Col[1];

	rtnInfo.strEx = QObject::tr("Failure to locate the second line!");

	if(findEdgePointDouble(Image, SecondLine, &Row, &Col, nEdge, L2R, nType,bMean) < 2)
	{
		return rtnInfo;
	}
	if (fRange>0)//自动纠错
	{
		testRtn=testEdgePointDouble(Image,SecondLine,Row,Col,nEdge,fRange,2,oldOri,nType,bMean);
		if(testRtn!=0)/*左右点都不正确时,缩放line重定位*/
		{		
			Hobject zoomLine;
			if (testRtn==1)//左右大于实际
			{
				gen_region_line(&zoomLine,Row[0].L(),Col[0].L(),Row[1].L(),Col[1].L());
				Row=HTuple();
				Col=HTuple();
				if (findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean)<2)
				{
					return rtnInfo;
				}
				if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,2,oldOri,nType,bMean)!=0)
				{
					return rtnInfo;
				}
			} else if (testRtn==2)//左右小于实际
			{
				int MoveDist=30;//左右扩展的长度
				double Phi;
				double rowLeft,colLeft,rowRight,colRight;
				line_orientation(Row[0].D(),Col[0].D(),Row[1].D(),Col[1].D(),&Phi);
				rowLeft=Row[0].D()+MoveDist*sin(Phi);
				colLeft=Col[0].D()-MoveDist*cos(Phi);
				rowRight=Row[1].D()-MoveDist*sin(Phi);
				colRight=Col[1].D()+MoveDist*cos(Phi);
				gen_region_line(&zoomLine,rowLeft,colLeft,rowRight,colRight);
				if (findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean)<2)
				{
					return rtnInfo;
				}
				if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,2,oldOri,nType,bMean)!=0)
				{
					return rtnInfo;
				}
			}
		}
	}
	nRowLeft2 = Row[0];
	nColLeft2 = Col[0];
	nRowRight2 = Row[1];
	nColRight2 = Col[1];

	newOri.nRow21 = Row[0];
	newOri.nCol21 = Col[0];
	newOri.nRow22 = Row[1];
	newOri.nCol22 = Col[1];	

	double MidRow1, MidCol1, MidRow2, MidCol2;
	MidRow1 = (nRowLeft1+nRowRight1)/2.0;
	MidCol1 = (nColLeft1+nColRight1)/2.0;
	MidRow2 = (nRowLeft2+nRowRight2)/2.0;
	MidCol2 = (nColLeft2+nColRight2)/2.0;

	Hobject MidLine;
	double OriAngle;
	line_orientation(MidRow1, MidCol1, MidRow2, MidCol2, &OriAngle);

	if (OriAngle < 0)
	{
		OriAngle = PI+OriAngle;
	}

	// r *Nr + c * Nc - d = 0
	// 查找第三条线
	rtnInfo.strEx = QObject::tr("Failure to locate the third line!");

	Hlong nRow1, nCol1, nRow2, nCol2;
	smallest_rectangle1(ThirdLine, &nRow1, &nCol1, &nRow2, &nCol2);

	double fCol1,fCol2;
	if (0 == MidRow2-MidRow1)
	{
		fCol1 = (MidCol1+MidCol2)/2.0;//可优化
		fCol2 = (MidCol1+MidCol2)/2.0;
	}
	else
	{
		fCol1 = MidCol1+(MidCol2-MidCol1)*(nRow1-MidRow1)/(MidRow2-MidRow1);
		fCol2 = MidCol1+(MidCol2-MidCol1)*(nRow2-MidRow1)/(MidRow2-MidRow1);
	}

	Hlong nWidth, nHeight;
	get_image_pointer1(Image, NULL, NULL, &nWidth, &nHeight);
	if (fCol1 < 0 || fCol1 > nWidth-1 || fCol2 < 0 || fCol2 > nWidth-1)
	{
		return rtnInfo;
	}

	//MJ备注-生成第一条线、第二条线中心点连线的延长线
	gen_region_line(&MidLine, nRow1, fCol1, nRow2, fCol2);

	// 由上到下，寻找从亮到暗的边界点
	if (!bFindPointSubPix)
	{
		if(findEdgePointSingle(Image, MidLine, &Row, &Col, nEdge, T2B, nType,bMean) != 1)
		{
			return rtnInfo;
		}
	}
	else //用亚像素法查找边界点
	{
		if(FindEdgePointSubPix(Image, MidLine, &Row, &Col, nEdge, T2B, nType) != 1)
		{
			return rtnInfo;
		}
	}

	nRow1 = Row[0];
	nCol1 = Col[0];

	newOri.Row = nRow1;
	newOri.Col = nCol1;
	newOri.Angle = OriAngle;

	rtnInfo.nType = GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	return rtnInfo;
}

//*功能：基于三条线计算瓶身原点（平移旋转-应力）
RtnInfo CCheck::findPosThreeLineStress(const Hobject &Image, const Hobject &FirstLine, const Hobject &SecondLine,const Hobject &ThirdLine,
	SideOrigin oldOri,SideOrigin &newOri,int nEdge,float fRange,int nType/*=0*/, bool bMean/*=FALSE*/)
{
	RtnInfo rtnInfo;

	double nRowLeft1, nColLeft1, nRowRight1, nColRight1;
	double nRowLeft2, nColLeft2, nRowRight2, nColRight2;
	double nRowTop, nColTop;
	HTuple Row, Col;
	HTuple row1, col1, row2, col2, row1_Temp, row2_Temp;
	HTuple LeftRow, LeftCol, RightRow, RightCol;
	Hobject rectTemp, ImgReduced, Edges, LeftEdge, RightEdge;
	Hlong num;
	//int testRtn;

	rtnInfo.nType = ERROR_LOCATEFAIL;
	rtnInfo.strEx = QObject::tr("Failure to locate the first line!");

	smallest_rectangle1(FirstLine, &row1, &col1, &row2, &col2);
	row1_Temp = (row1-5)<0?0:(row1-5);
	row2_Temp = (row2+5)>m_nHeight?m_nHeight:(row2+5);
	if (row2_Temp<row1_Temp || col2<col1)
	{
		return rtnInfo;
	}
	gen_rectangle1(&rectTemp, row1_Temp, col1, row2_Temp, col2);
	reduce_domain(Image, rectTemp, &ImgReduced);
	edges_sub_pix(ImgReduced, &Edges, "canny", 5, 10, 20);
	select_shape_xld(Edges, &Edges, "height", "and", 5, 99999);
	count_obj(Edges, &num);
	if(num < 2)
	{
		return rtnInfo;
	}
	sort_contours_xld(Edges, &Edges, "upper_left", "true", "column");
	select_obj(Edges, &LeftEdge, 1);
	select_obj(Edges, &RightEdge, num);
	get_contour_xld(LeftEdge, &LeftRow, &LeftCol);
	get_contour_xld(RightEdge, &RightRow, &RightCol);
	tuple_mean (LeftRow, &nRowLeft1);
	tuple_mean (LeftCol, &nColLeft1);
	tuple_mean (RightRow, &nRowRight1);
	tuple_mean (RightCol, &nColRight1);	

	newOri.nRow11 = nRowLeft1;
	newOri.nCol11 = nColLeft1;
	newOri.nRow12 = nRowRight1;
	newOri.nCol12 = nColRight1;

	rtnInfo.strEx = QObject::tr("Failure to locate the second line!");

	smallest_rectangle1(SecondLine, &row1, &col1, &row2, &col2);
	row1_Temp = (row1-5)<0?0:(row1-5);
	row2_Temp = (row2+5)>m_nHeight?m_nHeight:(row2+5);
	if (row2_Temp<row1_Temp || col2<col1)
	{
		return rtnInfo;
	}
	gen_rectangle1(&rectTemp, row1_Temp, col1, row2_Temp, col2);
	reduce_domain(Image, rectTemp, &ImgReduced);
	edges_sub_pix(ImgReduced, &Edges, "canny", 5, 10, 20);
	select_shape_xld(Edges, &Edges, "height", "and", 5, 99999);
	count_obj(Edges, &num);
	if(num < 2)
	{
		return rtnInfo;
	}
	sort_contours_xld(Edges, &Edges, "upper_left", "true", "column");
	select_obj(Edges, &LeftEdge, 1);
	select_obj(Edges, &RightEdge, num);
	get_contour_xld(LeftEdge, &LeftRow, &LeftCol);
	get_contour_xld(RightEdge, &RightRow, &RightCol);
	tuple_mean (LeftRow, &nRowLeft2);
	tuple_mean (LeftCol, &nColLeft2);
	tuple_mean (RightRow, &nRowRight2);
	tuple_mean (RightCol, &nColRight2);	

	newOri.nRow21 = nRowLeft2;
	newOri.nCol21 = nColLeft2;
	newOri.nRow22 = nRowRight2;
	newOri.nCol22 = nColRight2;	

	double MidRow1, MidCol1, MidRow2, MidCol2;
	MidRow1 = (nRowLeft1+nRowRight1)/2.0;
	MidCol1 = (nColLeft1+nColRight1)/2.0;
	MidRow2 = (nRowLeft2+nRowRight2)/2.0;
	MidCol2 = (nColLeft2+nColRight2)/2.0;

	Hobject MidLine;
	double OriAngle;
	line_orientation(MidRow1, MidCol1, MidRow2, MidCol2, &OriAngle);

	if (OriAngle < 0)
	{
		OriAngle = PI+OriAngle;
	}

	// 查找第三条线
	rtnInfo.strEx = QObject::tr("Failure to locate the third line!");

	Hlong nRow1, nCol1, nRow2, nCol2;
	smallest_rectangle1(ThirdLine, &nRow1, &nCol1, &nRow2, &nCol2);

	double fCol1,fCol2;
	if (0 == MidRow2-MidRow1)
	{
		fCol1 = (MidCol1+MidCol2)/2.0;
		fCol2 = (MidCol1+MidCol2)/2.0;
	}
	else
	{
		fCol1 = MidCol1+(MidCol2-MidCol1)*(nRow1-MidRow1)/(MidRow2-MidRow1);
		fCol2 = MidCol1+(MidCol2-MidCol1)*(nRow2-MidRow1)/(MidRow2-MidRow1);
	}

	Hlong nWidth, nHeight;
	get_image_pointer1(Image, NULL, NULL, &nWidth, &nHeight);
	if (fCol1 < 0 || fCol1 > nWidth-1 || fCol2 < 0 || fCol2 > nWidth-1)
	{
		return rtnInfo;
	}

	//MJ备注-生成第一条线、第二条线中心点连线的延长线
	gen_region_line(&MidLine, nRow1, fCol1, nRow2, fCol2);

	HTuple col1_Temp,col2_Temp,TopRow,TopCol;
	Hobject TopEdge;
	smallest_rectangle1(MidLine, &row1, &col1, &row2, &col2);
	col1_Temp = (col1-5)<0?0:(col1-5);
	col2_Temp = (col2+5)>m_nWidth?m_nWidth:(col2+5);
	if (row2<row1 || col2_Temp<col1_Temp)
	{
		return rtnInfo;
	}
	gen_rectangle1(&rectTemp, row1, col1_Temp, row2, col2_Temp);
	reduce_domain(Image, rectTemp, &ImgReduced);
	edges_sub_pix(ImgReduced, &Edges, "canny", 5, 10, 20);
	select_shape_xld(Edges, &Edges, "width", "and", 5, 99999);
	count_obj(Edges, &num);
	if(num < 1)
	{
		return rtnInfo;
	}
	sort_contours_xld(Edges, &Edges, "upper_left", "true", "row");
	select_obj(Edges, &TopEdge, 1);
	get_contour_xld(TopEdge, &TopRow, &TopCol);
	tuple_mean (TopRow, &nRowTop);
	tuple_mean (TopCol, &nColTop);

	newOri.Row = nRowTop;
	newOri.Col = nColTop;
	newOri.Angle = OriAngle;

	rtnInfo.nType = GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	return rtnInfo;
}


//*功能：基于两条横向线计算瓶身原点
RtnInfo CCheck::findXTwoLine(const Hobject &Image, const Hobject &FirstLine, const Hobject &SecondLine,
	SideOrigin &newOri, int nEdge, int nDirect,int nType/*=0*/,BOOL bMean/*=FALSE*/)
{
	Hlong Row1, Col1, Row2, Col2;
	HTuple Row, Col;
	Hobject RegLine;
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;
	rtnInfo.strEx = QObject::tr("Failure to locate the first line!");
	if (findEdgePointSingle(Image, FirstLine, &Row, &Col, nEdge, nDirect,nType,bMean) == 0)
	{
		return rtnInfo;
	}
	Row1 = Row[0];
	Col1 = Col[0];

	newOri.nRow11 = Row[0];
	newOri.nCol11 = Col[0];
	newOri.nRow12 = Row[0];
	newOri.nCol12 = Col[0];
	rtnInfo.strEx = QObject::tr("Failure to locate the second line!");
	if (findEdgePointSingle(Image, SecondLine, &Row, &Col, nEdge, nDirect, nType,bMean) == 0)
	{
		return rtnInfo;
	}
	Row2 = Row[0];
	Col2 = Col[0];

	newOri.nRow21 = Row[0];
	newOri.nCol21 = Col[0];
	newOri.nRow22 = Row[0];
	newOri.nCol22 = Col[0];

	double Phi;
	line_orientation(Row1, Col1, Row2, Col2, &Phi);

	if (Phi < 0)
	{
		Phi = PI+Phi;
	}

	newOri.Row = (Row1+Row2)/2.0;
	newOri.Col = (Col1+Col2)/2.0;
	newOri.Angle = Phi;

	rtnInfo.nType=GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	return rtnInfo;
}
//*功能：计算第三条线到第一、二条线边界点的直线的距离，用于单侧平移旋转定位
double CCheck::CalcDistPt3ToOri(const Hobject &Image, const Hobject &FirstLine, const Hobject &SecondLine,
	const Hobject &ThirdLine, int nEdge,int nDirect,int nType/*=0*/,BOOL bMean/*=FALSE*/)
{
	Hlong nRowLeft1, nColLeft1;
	Hlong nRowLeft2, nColLeft2;
	Hlong nRowTop1,nColTop1;
	HTuple Row, Col;
	double dist;

	if(findEdgePointSingle(Image,FirstLine, &Row, &Col, nEdge, nDirect, nType,bMean) < 1)
	{
		return -1;
	}

	nColLeft1 = Col[0];	
	nRowLeft1 = Row[0];	

	if(findEdgePointSingle(Image, SecondLine, &Row, &Col, nEdge, nDirect, nType,bMean) < 1)
	{
		return -1;
	}	

	nRowLeft2 = Row[0];
	nColLeft2 = Col[0];	

	if(findEdgePointSingle(Image, ThirdLine, &Row, &Col, nEdge, T2B, nType,bMean) < 1)
	{
		return -1;
	}
	nRowTop1=Row[0];
	nColTop1=Col[0];

	distance_pl(nRowTop1,nColTop1,nRowLeft1,nColLeft1,nRowLeft2,nColLeft2,&dist);
	return dist;
}
//*功能：通过单侧三条线计算瓶身原点
RtnInfo CCheck::findPosThreePoints(const Hobject &Image, const Hobject &FirstLine, const Hobject &SecondLine,
	const Hobject &ThirdLine,SideOrigin &newOri,int nEdge,int nDirect,int nType/*=0*/,BOOL bMean/*=FALSE*/)
{
	Hlong nRowLeft1, nColLeft1;
	Hlong nRowLeft2, nColLeft2;
	HTuple Row, Col;	
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;
	rtnInfo.strEx = QObject::tr("Failure to locate the first line!");
	if(findEdgePointSingle(Image,FirstLine, &Row, &Col, nEdge, nDirect, nType,bMean) < 1)
	{
		return rtnInfo;
	}

	nColLeft1 = Col[0];	
	nRowLeft1 = Row[0];	

	newOri.nRow11 = Row[0];
	newOri.nCol11 = Col[0];
	newOri.nRow12 = Row[0];
	newOri.nCol12 = Col[0];

	rtnInfo.strEx = QObject::tr("Failure to locate the second line!");
	if(findEdgePointSingle(Image, SecondLine, &Row, &Col, nEdge, nDirect, nType,bMean) < 1)
	{
		return rtnInfo;
	}	

	nRowLeft2 = Row[0];
	nColLeft2 = Col[0];

	newOri.nRow21 = Row[0];
	newOri.nCol21 = Col[0];	
	newOri.nRow22 = Row[0];
	newOri.nCol22 = Col[0];

	if (nRowLeft2 == nRowLeft1)
	{
		return rtnInfo;
	}

	//查找第三条线 
	Hobject MidLine;
	double OriAngle;
	line_orientation(nRowLeft1, nColLeft1, nRowLeft2, nColLeft2, &OriAngle); 

	if (OriAngle < 0)
	{
		OriAngle = PI+OriAngle;
	}

	Hlong nRow1, nCol1, nRow2, nCol2;
	HTuple RowTemp;	
	smallest_rectangle1(ThirdLine, &nRow1, &nCol1, &nRow2, &nCol2);	
	double fCol1,fCol2;
	fCol1 = nColLeft1+(nColLeft2-nColLeft1)*(nRow1-nRowLeft1)/(nRowLeft2-nRowLeft1);
	fCol2 = nColLeft1+(nColLeft2-nColLeft1)*(nRow2-nRowLeft1)/(nRowLeft2-nRowLeft1);
	Hlong nWidth, nHeight;
	get_image_pointer1(Image, NULL, NULL, &nWidth, &nHeight);
	rtnInfo.strEx = QObject::tr("Failure to locate the third line!");
	if (fCol1 < 0 || fCol1 > nWidth-1 || fCol2 < 0 || fCol2 > nWidth-1)
	{
		return rtnInfo;
	}
	//平移第三条线

	switch(nDirect)
	{
	case L2R:
		nRow1+=newOri.fDist3*sin(PI/2-OriAngle);
		fCol1+=newOri.fDist3*cos(PI/2-OriAngle);
		nRow2+=newOri.fDist3*sin(PI/2-OriAngle);
		fCol2+=newOri.fDist3*cos(PI/2-OriAngle);
		break;
	case R2L:
		nRow1-=newOri.fDist3*sin(PI/2-OriAngle);
		fCol1-=newOri.fDist3*cos(PI/2-OriAngle);
		nRow2-=newOri.fDist3*sin(PI/2-OriAngle);
		fCol2-=newOri.fDist3*cos(PI/2-OriAngle);	
	default:
		break;
	}

	gen_region_line(&MidLine, nRow1, fCol1, nRow2, fCol2);

	if(findEdgePointSingle(Image, MidLine, &Row, &Col, nEdge, T2B, nType,bMean) < 1)
	{
		return rtnInfo;
	}
	nRow1=Row[0];
	nCol1=Col[0];

	double slope=tan(OriAngle);
	double OriRow,OriCol;
	double Row10=20/slope+nRow1;
	intersection_ll(nRowLeft1,nColLeft1,nRowLeft2,nColLeft2,nRow1,nCol1,Row10,nCol1+20,&OriRow,&OriCol,NULL);

	newOri.Row = OriRow;
	newOri.Col = OriCol;
	newOri.Angle = OriAngle;	
	rtnInfo.nType = GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	return rtnInfo;
}
//*功能：获取区域内瓶子轮廓，用于应力定位
void CCheck::getBottleXld(const Hobject &Image,const Hobject &bottleRegion,Hobject &bottleXld,int xldwidth,int xldheight,int nDirect)
{
	//nDirect; 1-取左边 2-取右边 3-取上边
	//边缘提取瓶子轮廓参数
	int nThresh = 10;
	Hobject ImgPart,ImageSobel;
	Hobject tempRegion,edgeRegs;	
	Hlong edgeNums;
	HTuple edgeCol;
	int i,count,lastRow,lastCol,minCol=9999,maxCol=0,edgeIdx=0;
	Hlong edgeArea;
	HTuple rPtTemp,cPtTemp,rowPts,colPts;
	//阈值提取瓶子轮廓参数
	double bottleThresh;
	Hobject RegThres, RegErosion, RegCon,RegSkl;
	Hobject ImgRect;
	Hlong nNumber = 0;
	HTuple Length, Indices;
	gen_empty_obj(&bottleXld);

	switch(nDirect)
	{
	case 1:
		//边缘取最左边
		reduce_domain(Image, bottleRegion, &ImgPart);	
		sobel_amp(ImgPart,&ImageSobel,"sum_abs",3);
		threshold(ImageSobel,&tempRegion,nThresh,255);	
		connection(tempRegion,&tempRegion);
		closing_rectangle1(tempRegion,&tempRegion,3,8);//连接边缘，太大会漏报0208			
		select_shape(tempRegion,&tempRegion,HTuple("height"),"and",HTuple(xldheight),HTuple(99999));
		count_obj(tempRegion,&edgeNums);
		if (edgeNums==0)
			return;//未找到瓶子轮廓
		smallest_rectangle1(tempRegion,NULL,&edgeCol,NULL,NULL);
		//area_center(tempRegion,NULL,NULL,&edgeCol);
		rowPts=HTuple();
		colPts=HTuple();
		for (i=0;i<edgeNums;i++)
		{
			if (edgeCol[i].I()<minCol)
			{
				minCol=edgeCol[i].I();
				edgeIdx=i;
			}
		}
		select_obj(tempRegion,&edgeRegs,edgeIdx+1);
		area_center(edgeRegs,&edgeArea,NULL,NULL);
		if (edgeArea<5)
			return;//搜索到的边缘太小	
		//边缘连续
		get_region_points(edgeRegs,&rPtTemp,&cPtTemp);
		//取区域左侧
		lastRow=rPtTemp[0].L()-1;		
		for (i=0;i<rPtTemp.Num();i++)
		{			
			if ((rPtTemp[i].L()-lastRow)>0)
			{
				lastRow=rPtTemp[i].L();
				rowPts.Append(rPtTemp[i]);
				colPts.Append(cPtTemp[i]);	
			}				
		}
		count=rowPts.Num();
		if (count>5)
			gen_contour_polygon_xld(&bottleXld,rowPts,colPts);
		break;
	case 2:
		//边缘取最右边
		reduce_domain(Image, bottleRegion, &ImgPart);	
		sobel_amp(ImgPart,&ImageSobel,"sum_abs",3);
		threshold(ImageSobel,&tempRegion,nThresh,255);	
		connection(tempRegion,&tempRegion);
		closing_rectangle1(tempRegion,&tempRegion,3,8);//连接边缘，太大会漏报0208			
		select_shape(tempRegion,&tempRegion,HTuple("height"),"and",HTuple(xldheight),HTuple(99999));
		count_obj(tempRegion,&edgeNums);
		if (edgeNums==0)
			return;//未找到瓶子轮廓

		smallest_rectangle1(tempRegion,NULL,NULL,NULL,&edgeCol);
		//area_center(tempRegion,NULL,NULL,&edgeCol);
		rowPts=HTuple();
		colPts=HTuple();
		for (i=0;i<edgeNums;i++)
		{
			if (edgeCol[i].I()>maxCol)
			{
				maxCol=edgeCol[i].I();
				edgeIdx=i;
			}
		}
		select_obj(tempRegion,&edgeRegs,edgeIdx+1);
		area_center(edgeRegs,&edgeArea,NULL,NULL);
		if (edgeArea<5)
			return;//搜索到的边缘太小	
		//边缘连续
		get_region_points(edgeRegs,&rPtTemp,&cPtTemp);
		//取区域右侧
		lastRow=rPtTemp[0].L(),lastCol=cPtTemp[0].L()-1;

		for (i=0;i<rPtTemp.Num();i++)
		{//注意与左边的区别：取区域右边缘
			if ((rPtTemp[i].L()-lastRow)>=0)
			{
				if ((rPtTemp[i].L()-lastRow)>0)
				{
					rowPts.Append(lastRow);
					colPts.Append(lastCol);	
				}
				lastRow=rPtTemp[i].L();
				lastCol=cPtTemp[i].L();
			}				
		}
		count=rowPts.Num();
		if (count>5)
			gen_contour_polygon_xld(&bottleXld,rowPts,colPts);
		break;
	case 3:
		//阈值取上边
		intensity(bottleRegion,Image,&bottleThresh,NULL);
		reduce_domain(Image, bottleRegion, &ImgRect);
		threshold(ImgRect, &RegThres, 0, bottleThresh);
		connection(RegThres, &RegThres);
		select_shape(RegThres, &RegThres, HTuple("width").Concat("height"), "and",
			HTuple(0.9*xldwidth).Concat(0.9*xldheight),HTuple(99999).Concat(99999));
		count_obj(RegThres, &nNumber);
		if (nNumber > 0)
		{
			select_shape_std(RegThres, &RegThres, "max_area", 70);
			fill_up(RegThres, &RegThres);
			boundary(RegThres,&RegThres,"outer");
			erosion_circle(bottleRegion,&RegErosion,3.5);
			intersection(RegErosion,RegThres,&RegThres);
			connection (RegThres, &RegCon);
			select_shape(RegCon, &RegCon, HTuple("width").Concat("height"), "and",
				HTuple(0.9*xldwidth).Concat(0.9*xldheight),HTuple(99999).Concat(99999));
			count_obj (RegCon,&nNumber);
			if(nNumber>0)
			{
				dilation_circle (RegCon, &RegCon, 3.5);
				select_shape_std (RegCon, &RegCon, "max_area", 70);
				skeleton (RegCon, &RegSkl);
				gen_contours_skeleton_xld (RegSkl, &bottleXld, 1, "filter");
				count_obj (bottleXld, &nNumber);
				if(nNumber>1)   
				{
					length_xld (bottleXld, &Length);
					tuple_sort_index (Length, &Indices);
					select_obj (bottleXld, &bottleXld, Indices[nNumber-1].I()+1);
				}
			}
		}
	default:
		break;
	}
}
//*功能：基于两条线(横向纵向)线计算平移，不旋转
RtnInfo CCheck::findXYTwoLine(const Hobject &Image, const Hobject &FirstLine, const Hobject &ThirdLine,
	SideOrigin oldOri,SideOrigin &newOri, int nEdge,float fRange,int nType/*=0*/,BOOL bMean/*=FALSE*/,BOOL bFindPointSubPix/*=FALSE*/)
{
	RtnInfo rtnInfo;

	Hlong nRowLeft1, nColLeft1, nRowRight1, nColRight1;
	HTuple Row, Col;
	int testRtn;
	Hobject MidLine;

	rtnInfo.nType = ERROR_LOCATEFAIL;
	rtnInfo.strEx = QObject::tr("Failure to locate the first line!");

	if(findEdgePointDouble(Image, FirstLine, &Row, &Col, nEdge, L2R, nType,bMean) < 2)
	{
		return rtnInfo;
	}

	if (fRange>0)//自动纠错
	{
		testRtn=testEdgePointDouble(Image,FirstLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean);
		if(testRtn!=0)/*左右点都不正确时,缩放line重定位*/
		{		
			Hobject zoomLine;
			if (testRtn==1)//左右大于实际
			{
				gen_region_line(&zoomLine,Row[0].L(),Col[0].L(),Row[1].L(),Col[1].L());
				Row=HTuple();
				Col=HTuple();
				if (findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean)<2)
				{					
					return rtnInfo;
				}
				if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean)!=0)
				{					
					return rtnInfo;
				}
			} else if (testRtn==2)//左右小于实际
			{
				int MoveDist=30;//左右扩展的长度
				double Phi;
				double rowLeft,colLeft,rowRight,colRight;
				line_orientation(Row[0].D(),Col[0].D(),Row[1].D(),Col[1].D(),&Phi);
				rowLeft=Row[0].D()+MoveDist*sin(Phi);
				colLeft=Col[0].D()-MoveDist*cos(Phi);
				rowRight=Row[1].D()-MoveDist*sin(Phi);
				colRight=Col[1].D()+MoveDist*cos(Phi);
				gen_region_line(&zoomLine,rowLeft,colLeft,rowRight,colRight);
				if (findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean)<2)
				{
					return rtnInfo;
				}
				if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean)!=0)
				{
					return rtnInfo;
				}
			}
		}
	}

	nColLeft1 = Col[0];
	nColRight1 = Col[1];
	nRowLeft1 = Row[0];
	nRowRight1 = Row[1];

	newOri.nRow11 = Row[0];
	newOri.nCol11 = Col[0];
	newOri.nRow12 = Row[1];
	newOri.nCol12 = Col[1];

	newOri.nRow21 = Row[0];
	newOri.nCol21 = Col[0];
	newOri.nRow22 = Row[1];
	newOri.nCol22 = Col[1];

	double MidCol1;
	MidCol1 = (nColLeft1+nColRight1)/2.0;

	rtnInfo.strEx = QObject::tr("Failure to locate the third line!");
	Hlong nRow1, nCol1, nRow2, nCol2;
	smallest_rectangle1(ThirdLine, &nRow1, &nCol1, &nRow2, &nCol2);

	gen_region_line(&MidLine, nRow1, MidCol1, nRow2, MidCol1);

	// 由上到下，寻找从亮到暗的边界点
	if (!bFindPointSubPix)
	{
		if(findEdgePointSingle(Image, MidLine, &Row, &Col, nEdge, T2B, nType,bMean) != 1)
		{
			return rtnInfo;
		}
	} 
	else //用亚像素的方法查找边界点
	{
		if(FindEdgePointSubPix(Image, MidLine, &Row, &Col, nEdge, T2B, nType) != 1)
		{
			return rtnInfo;
		}
	}

	nRow1 = Row[0];
	nCol1 = Col[0];

	newOri.Row = nRow1;
	newOri.Col = nCol1;
	newOri.Angle = 0;

	rtnInfo.nType = GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	return rtnInfo;
}
//*功能：基于两条横向线计算瓶身原点,添加连瓶判断
RtnInfo CCheck::findXTwoLineConBottle(const Hobject &Image, const Hobject &FirstLine, const Hobject &SecondLine,
	SideOrigin oldOri,SideOrigin &newOri, int nEdge,float fRange,int nDirect,int nType/*=0*/,BOOL bMean/*=FALSE*/)
{
	RtnInfo rtnInfo;

	Hlong nRowLeft1, nColLeft1, nRowRight1, nColRight1;
	Hlong nRowLeft2, nColLeft2, nRowRight2, nColRight2;
	HTuple Row, Col;
	int testRtn,nEdgePointNum;
	bool bSingle;

	rtnInfo.nType = ERROR_LOCATEFAIL;
	rtnInfo.strEx = QObject::tr("Failure to locate the first line!");
	bSingle = false;
	nEdgePointNum = findEdgePointDouble(Image, FirstLine, &Row, &Col, nEdge, L2R, nType,bMean);
	if(nEdgePointNum < 2)
	{
		if (nEdgePointNum == 1 && (nDirect ==1 || nDirect == 2))
		{
			if (findEdgePointSingle(Image, FirstLine, &Row, &Col, nEdge, nDirect,nType,bMean) == 0)
			{
				return rtnInfo;
			}
			else
			{
				bSingle = true;
			}
		}
		else
		{
			return rtnInfo;
		}
	}
	if (fRange>0 && nEdgePointNum>1)//自动纠错
	{
		testRtn=testEdgePointDouble(Image,FirstLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean);
		if(testRtn!=0)/*左右点都不正确时,缩放line重定位*/
		{		
			Hobject zoomLine;
			if (testRtn==1)//左右大于实际
			{
				gen_region_line(&zoomLine,Row[0].L(),Col[0].L(),Row[1].L(),Col[1].L());
				Row=HTuple();
				Col=HTuple();
				nEdgePointNum = findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean);
				if (nEdgePointNum<2)
				{					
					if (nEdgePointNum == 1 && (nDirect ==1 || nDirect == 2))
					{
						if (findEdgePointSingle(Image, zoomLine, &Row, &Col, nEdge, nDirect,nType,bMean) == 0)
						{
							return rtnInfo;
						}
						else
						{
							bSingle = true;
						}
					}
					else
					{
						return rtnInfo;
					}
				}
				if (nDirect ==3)
				{
					if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean)!=0)
					{					
						return rtnInfo;
					}
				}
			} else if (testRtn==2)//左右小于实际
			{
				int MoveDist=30;//左右扩展的长度
				double Phi;
				double rowLeft,colLeft,rowRight,colRight;
				line_orientation(Row[0].D(),Col[0].D(),Row[1].D(),Col[1].D(),&Phi);
				rowLeft=Row[0].D()+MoveDist*sin(Phi);
				colLeft=Col[0].D()-MoveDist*cos(Phi);
				rowRight=Row[1].D()-MoveDist*sin(Phi);
				colRight=Col[1].D()+MoveDist*cos(Phi);
				gen_region_line(&zoomLine,rowLeft,colLeft,rowRight,colRight);
				nEdgePointNum = findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean);
				if (nEdgePointNum<2)
				{
					if (nEdgePointNum == 1 && (nDirect ==1 || nDirect == 2))
					{
						if (findEdgePointSingle(Image, zoomLine, &Row, &Col, nEdge, nDirect,nType,bMean) == 0)
						{
							return rtnInfo;
						}
						else
						{
							bSingle = true;
						}
					}
					else
					{
						return rtnInfo;
					}
				}
				if (nDirect ==3)
				{
					if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,1,oldOri,nType,bMean)!=0)
					{
						return rtnInfo;
					}
				}
			}
		}
	}
	if (bSingle)
	{
		nColLeft1 = Col[0];
		nRowLeft1 = Row[0];
		nColRight1 = Col[0];
		nRowRight1 = Row[0];
	}
	else
	{
		nColLeft1 = Col[0];
		nRowLeft1 = Row[0];
		nColRight1 = Col[1];
		nRowRight1 = Row[1];
	}
	newOri.nRow11 = nRowLeft1;
	newOri.nCol11 = nColLeft1;
	newOri.nRow12 = nRowRight1;
	newOri.nCol12 = nColRight1;


	rtnInfo.strEx = QObject::tr("Failure to locate the second line!");
	bSingle = false;
	nEdgePointNum = findEdgePointDouble(Image, SecondLine, &Row, &Col, nEdge, L2R, nType,bMean);
	if(nEdgePointNum < 2)
	{
		if (nEdgePointNum == 1 && (nDirect ==1 || nDirect == 2))
		{
			if (findEdgePointSingle(Image, SecondLine, &Row, &Col, nEdge, nDirect,nType,bMean) == 0)
			{
				return rtnInfo;
			}
			else
			{
				bSingle = true;
			}
		}
		else
		{
			return rtnInfo;
		}
	}
	if (fRange>0 && nEdgePointNum>1)//自动纠错
	{
		testRtn=testEdgePointDouble(Image,SecondLine,Row,Col,nEdge,fRange,2,oldOri,nType,bMean);
		if(testRtn!=0)/*左右点都不正确时,缩放line重定位*/
		{		
			Hobject zoomLine;
			if (testRtn==1)//左右大于实际
			{
				gen_region_line(&zoomLine,Row[0].L(),Col[0].L(),Row[1].L(),Col[1].L());
				Row=HTuple();
				Col=HTuple();
				nEdgePointNum = findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean);
				if (nEdgePointNum<2)
				{
					if (nEdgePointNum == 1 && (nDirect ==1 || nDirect == 2))
					{
						if (findEdgePointSingle(Image, zoomLine, &Row, &Col, nEdge, nDirect,nType,bMean) == 0)
						{
							return rtnInfo;
						}
						else
						{
							bSingle = true;
						}
					}
					else
					{
						return rtnInfo;
					}
				}
				if (nDirect ==3)
				{
					if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,2,oldOri,nType,bMean)!=0)
					{
						return rtnInfo;
					}
				}
			} else if (testRtn==2)//左右小于实际
			{
				int MoveDist=30;//左右扩展的长度
				double Phi;
				double rowLeft,colLeft,rowRight,colRight;
				line_orientation(Row[0].D(),Col[0].D(),Row[1].D(),Col[1].D(),&Phi);
				rowLeft=Row[0].D()+MoveDist*sin(Phi);
				colLeft=Col[0].D()-MoveDist*cos(Phi);
				rowRight=Row[1].D()-MoveDist*sin(Phi);
				colRight=Col[1].D()+MoveDist*cos(Phi);
				gen_region_line(&zoomLine,rowLeft,colLeft,rowRight,colRight);
				nEdgePointNum = findEdgePointDouble(Image,zoomLine,&Row,&Col,nEdge,L2R,nType,bMean);
				if (nEdgePointNum<2)
				{
					if (nEdgePointNum == 1 && (nDirect ==1 || nDirect == 2))
					{
						if (findEdgePointSingle(Image, zoomLine, &Row, &Col, nEdge, nDirect,nType,bMean) == 0)
						{
							return rtnInfo;
						}
						else
						{
							bSingle = true;
						}
					}
					else
					{
						return rtnInfo;
					}
				}
				if (nDirect ==3)
				{
					if (testEdgePointDouble(Image,zoomLine,Row,Col,nEdge,fRange,2,oldOri,nType,bMean)!=0)
					{
						return rtnInfo;
					}
				}
			}
		}
	}

	if (bSingle)
	{
		nColLeft2 = Col[0];
		nRowLeft2 = Row[0];
		nColRight2 = Col[0];
		nRowRight2 = Row[0];
	}
	else
	{
		nColLeft2 = Col[0];
		nRowLeft2 = Row[0];
		nColRight2 = Col[1];
		nRowRight2 = Row[1];
	}
	newOri.nRow21 = nRowLeft2;
	newOri.nCol21 = nColLeft2;
	newOri.nRow22 = nRowRight2;
	newOri.nCol22 = nColRight2;	

	double MidRow1, MidCol1, MidRow2, MidCol2;
	Hobject MidLine;
	double OriAngle;
	switch(nDirect)
	{
	case 1:
		newOri.Row = (nRowLeft1+nRowLeft2)/2;
		newOri.Col = (nColLeft1+nColLeft2)/2;
		line_orientation(nRowLeft1, nColLeft1, nRowLeft2, nColLeft2, &OriAngle);
		break;
	case 2:
		newOri.Row = (nRowRight1+nRowRight2)/2;
		newOri.Col = (nColRight1+nColRight2)/2;
		line_orientation(nRowRight1, nColRight1, nRowRight2, nColRight2, &OriAngle);
		break;
	case 3:
		MidRow1 = (nRowLeft1+nRowRight1)/2.0;
		MidCol1 = (nColLeft1+nColRight1)/2.0;
		MidRow2 = (nRowLeft2+nRowRight2)/2.0;
		MidCol2 = (nColLeft2+nColRight2)/2.0;
		newOri.Row = (MidRow1+MidRow2)/2;
		newOri.Col = (MidCol1+MidCol2)/2;
		line_orientation(MidRow1, MidCol1, MidRow2, MidCol2, &OriAngle);
		break;
	}
	if (OriAngle < 0)
	{
		OriAngle = PI+OriAngle;
	}
	newOri.Angle = OriAngle;

	rtnInfo.nType=GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	return rtnInfo;

}
//*功能：瓶口定位
RtnInfo CCheck::fnFindPosFinish(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pFinLoc pFinLoc = para.value<s_pFinLoc>();
	s_oFinLoc oFinLoc = shape.value<s_oFinLoc>();
	switch(pFinLoc.nMethodIdx)
	{
	case 0:
		rtnInfo = findPosRingLight(imgSrc,oFinLoc,pFinLoc);
		break;
	case 1:
		rtnInfo = findPosBackLight(imgSrc,oFinLoc,pFinLoc);
		break;
	case 2:
		rtnInfo = findPosRingLight_ZhangYu(imgSrc,oFinLoc,pFinLoc);
		break;
	default:
		break;
	}
	shape.setValue(oFinLoc);
	if (rtnInfo.nType>0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	}
	else
	{
		currentOri.Row = oFinLoc.Row;
		currentOri.Col = oFinLoc.Col;
	}
	return rtnInfo;
}
//*功能：环形光瓶口定位
RtnInfo CCheck::findPosRingLight(const Hobject &Image,s_oFinLoc &oFinLoc,s_pFinLoc &pFinLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hlong nNum;
	int nEdge = pFinLoc.nEdge;
	float fOpenSize = pFinLoc.fOpenSize;
	double dInRadii,dOutRadii;
	int nMaskSize = 31;
	int nBoundOut = 25;//搜索圆环的允许浮动上下限
	int nBoundIn  = 20;//20121217 100->25,50->20
	double dGrayMean;
	double Row, Column, Radius;	
	Hobject imgMean,regDynThresh,regOpening,regConnect,regSelect;
	Hobject RegCircu, ContSel;

	smallest_circle(oFinLoc.oInCircle,NULL,NULL,&dInRadii);
	smallest_circle(oFinLoc.oOutCircle,NULL,NULL,&dOutRadii);
	dInRadii=dOutRadii>dInRadii?dInRadii:dOutRadii-10;//防止内环比外环大
	mean_image(Image,&imgMean,nMaskSize,nMaskSize);
	intensity(oFinLoc.oInCircle,Image,&dGrayMean,NULL);
	if (dGrayMean > 200)
	{
		rtnInfo.strEx = QObject::tr("Bottle is too bright to check capsule!"); 
		return rtnInfo;
	}

	dyn_threshold(Image,imgMean,&regDynThresh,nEdge,"light");
	closing_circle(regDynThresh,&regDynThresh,3.5);//20121217增加
	opening_circle(regDynThresh,&regOpening,fOpenSize);
	connection(regOpening,&regConnect);

	//选取近似圆环的区域
	select_shape(regConnect, &regSelect, HTuple("width").Concat("height"), "and",
		HTuple(2*dInRadii-nBoundIn).Concat(2*dInRadii-nBoundIn),
		HTuple(2*dOutRadii+nBoundOut).Concat(2*dOutRadii+nBoundOut));
	select_shape(regSelect,&regSelect,"anisometry","and",0.7,1.3);//20120609防止边缘干扰定位误差大
	count_obj(regSelect, &nNum);
	if (nNum == 0)
	{
		//2015.1.27 肇庆改用内圈定位，定位失败后扩大半径，找外环定位
		int iRadiiEx = 60;
		select_shape(regConnect, &regSelect, HTuple("width").Concat("height"), "and",
			HTuple(2*dInRadii-nBoundIn).Concat(2*dInRadii-nBoundIn),
			HTuple(2*(dOutRadii+iRadiiEx)+nBoundOut).Concat(2*(dOutRadii+iRadiiEx)+nBoundOut));
		select_shape(regSelect,&regSelect,"anisometry","and",0.7,1.3);//20120609防止边缘干扰定位误差大
		count_obj(regSelect, &nNum);
		if (nNum == 0)
		{
			//2014.1.5当开运算尺度大于1.5时，若定位失败，则降低开运算尺度重新定位
			//rtnInfo.strEx = QObject::tr("Please check that whether inner or outter region is appropriate!"); 
			//return rtnInfo;
			if (fOpenSize >1.5)
			{
				opening_circle(regDynThresh,&regOpening,1.5);
				connection(regOpening,&regConnect);

				//选取近似圆环的区域
				select_shape(regConnect, &regSelect, HTuple("width").Concat("height"), "and",
					HTuple(2*dInRadii-nBoundIn).Concat(2*dInRadii-nBoundIn),
					HTuple(2*(dOutRadii+iRadiiEx)+nBoundOut).Concat(2*(dOutRadii+iRadiiEx)+nBoundOut));
				select_shape(regSelect,&regSelect,"anisometry","and",0.7,1.3);//20120609防止边缘干扰定位误差大
				count_obj(regSelect, &nNum);
				if (nNum == 0)
				{
					rtnInfo.strEx = QObject::tr("Please check that whether inner or outter region is appropriate!"); 
					return rtnInfo;
				}
			}
		}
	}

	union1(regSelect, &RegCircu);
	closing_circle(RegCircu, &RegCircu, 35.5);
	fill_up(RegCircu, &RegCircu);
	opening_circle(RegCircu, &RegCircu, 35.5);
	select_shape(RegCircu, &RegCircu, "area", "and", 1000, 99999999);
	select_shape(RegCircu, &RegCircu, "circularity", "and", 0.6, 1);
	count_obj(RegCircu, &nNum);
	//以上方法不能定位时
	if (nNum == 0)
	{
		Hobject Contours, SelectedXLD, UnionContours;
		HTuple Circula, Indices, Len;
		fill_up(regSelect, &RegCircu);
		gen_contour_region_xld(RegCircu, &Contours, "border");
		segment_contours_xld(Contours, &Contours, "lines_circles", 5, 4, 2);
		//防止外围圆形小凸起，导致后面选取轮廓时错误，50->100
		select_contours_xld(Contours, &SelectedXLD, "contour_length", 100, 99999999, 100, 99999999);
		count_obj(SelectedXLD, &nNum);
		if (nNum == 0)
		{
			return rtnInfo;
		}

		union_cocircular_contours_xld (SelectedXLD, &UnionContours, 0.5, 0.1, 0.2, 30, 10, 10, "true", 1);
		circularity_xld(UnionContours, &Circula);
		tuple_sort_index(Circula, &Indices);
		tuple_length(Indices, &Len);
		nNum = Len[0];
		if (nNum == 0)
		{
			return rtnInfo;
		}

		double Cir;
		Hlong Index;
		Index = Indices[nNum-1];
		Cir = Circula[Index];
		if (Cir < 0.1)
		{
			return rtnInfo;
		}
		select_obj(UnionContours, &ContSel, Index+1);	
	}
	else
	{
		select_shape_std(RegCircu, &RegCircu, "max_area", 70);
		gen_contour_region_xld(RegCircu, &ContSel, "border");
	}

	fit_circle_contour_xld (ContSel, "algebraic", -1, 0, 0, 3, 2, &Row, &Column, &Radius, NULL, NULL, NULL);
	oFinLoc.Row = Row;
	oFinLoc.Col = Column;
	oFinLoc.Radius = Radius;

	gen_circle(&oFinLoc.oInCircle,Row,Column,dInRadii);
	gen_circle(&oFinLoc.oOutCircle,Row,Column,dOutRadii);
	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}
//*功能：背光瓶口定位
RtnInfo CCheck::findPosBackLight(const Hobject &Image,s_oFinLoc &oFinLoc,s_pFinLoc &pFinLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hobject regCenter,contCenter;
	Hlong nNum;
	double Row,Col,Radius;

	double dInRadii;
	double dCentThresh;
	float fOriFloat = pFinLoc.nFloatRange/100.f;
	smallest_circle(oFinLoc.oInCircle,NULL,NULL,&dInRadii);
	intensity(oFinLoc.oInCircle,Image,&dCentThresh,NULL);
	dCentThresh-=20;

	fast_threshold(Image,&regCenter,dCentThresh,255,100);
	connection(regCenter,&regCenter);
	select_shape(regCenter,&regCenter,HTuple("circularity").Concat("height").Concat("width"),"and",
		HTuple(0.7).Concat(2*dInRadii*(1-fOriFloat)).Concat(2*dInRadii*(1-fOriFloat)),
		HTuple(1.0).Concat(2*dInRadii*(1+fOriFloat)).Concat(2*dInRadii*(1+fOriFloat)));
	closing_circle(regCenter,&regCenter,35.5);
	fill_up(regCenter,&regCenter);
	opening_circle(regCenter,&regCenter,35.5);
	count_obj(regCenter,&nNum);
	if (nNum!=1)
	{
		rtnInfo.strEx = QObject::tr("Inner radius is likely to exceed the floating range!");
		return rtnInfo;
	}

	gen_contour_region_xld(regCenter,&contCenter,"border");
	fit_circle_contour_xld(contCenter,"algebraic",-1, 0, 0, 3, 2, &Row,&Col,&Radius,NULL,NULL,NULL);

	gen_circle(&oFinLoc.oInCircle,Row,Col,dInRadii);
	oFinLoc.Row = Row;
	oFinLoc.Col = Col;
	oFinLoc.Radius = Radius;

	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}

////*功能：环形光瓶口定位（张裕）---2017.4
//RtnInfo CCheck::findPosRingLight_ZhangYu(const Hobject &Image,s_oFinLoc &oFinLoc,s_pFinLoc &pFinLoc)
//{
//	RtnInfo rtnInfo;
//	rtnInfo.nType = ERROR_LOCATEFAIL;
//
//	Hlong nNum;
//	double dInRadii,dOutRadii,dOpenRadius;
//	int nBoundOut = 25;//搜索圆环的允许浮动上下限
//	int nBoundIn  = 20;
//	double Row,Column,Radius;	
//	Hobject ImaAmp,ImaDir,LightReg,RegClosing,LightRegCon,regSelect;
//	Hobject RegUnion,RegOpening,RegCircu, ContSel;
//
//	smallest_circle(oFinLoc.oInCircle,NULL,NULL,&dInRadii);
//	smallest_circle(oFinLoc.oOutCircle,NULL,NULL,&dOutRadii);
//	dInRadii=dOutRadii>dInRadii?dInRadii:dOutRadii-10;//防止内环比外环大
//
//	edges_image (Image, &ImaAmp, &ImaDir, "canny", 1, "nms", 5, 10);
//	threshold (ImaAmp, &LightReg, 0, 255);
//	closing_circle (LightReg, &RegClosing, 5);
//	connection (RegClosing, &LightRegCon);
//	select_shape (LightRegCon, &regSelect,  HTuple("width").Concat("height"), "and",
//		HTuple(2*dInRadii-nBoundIn).Concat(2*dInRadii-nBoundIn),
//		HTuple(2*dOutRadii+nBoundOut).Concat(2*dOutRadii+nBoundOut));
//	count_obj (regSelect, &nNum);
//	if (nNum == 0)
//	{
//		return rtnInfo;
//	}
//
//	union1(regSelect, &RegUnion);
//	closing_circle(RegUnion, &RegClosing, 35.5);
//	fill_up(RegClosing, &RegClosing);
//	dOpenRadius = dInRadii-50;
//	dOpenRadius>35.5?dOpenRadius:35.5;
//	opening_circle(RegClosing, &RegOpening, dOpenRadius);
//	connection (RegOpening, &LightRegCon);
//	select_shape(LightRegCon, &regSelect, "area", "and", 1000, 99999999);
//	select_shape(regSelect, &regSelect, "circularity", "and", 0.6, 1);
//	count_obj(regSelect, &nNum);
//	if (nNum == 0)
//	{
//		return rtnInfo;
//	}
//
//	select_shape_std(regSelect, &RegCircu, "max_area", 70);
//	gen_contour_region_xld(RegCircu, &ContSel, "border");
//	fit_circle_contour_xld (ContSel, "algebraic", -1, 0, 0, 3, 2, &Row, &Column, &Radius, NULL, NULL, NULL);
//	oFinLoc.Row = Row;
//	oFinLoc.Col = Column;
//	oFinLoc.Radius = Radius;
//
//	gen_circle(&oFinLoc.oInCircle,Row,Column,dInRadii);
//	gen_circle(&oFinLoc.oOutCircle,Row,Column,dOutRadii);
//	rtnInfo.nType = GOOD_BOTTLE;
//	return rtnInfo;
//}

//*功能：环形光瓶口定位（张裕）---2017.5修改为瓶口外环定位
RtnInfo CCheck::findPosRingLight_ZhangYu(const Hobject &Image,s_oFinLoc &oFinLoc,s_pFinLoc &pFinLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hlong nNum,area,LowThres,HighThres;
	double dInRadii,dOutRadii,dOpenRadius,Low,High;
	double Row,Column,Radius;	
	Hobject ImaAmp,ImaDir,LightReg,RegClosing,RegDiff,RegDiffCon,regSelect;
	Hobject RegOpening,RegMouth,SortedRegions,RegMouthClo,RegFillup,RegMouthOutCir;
	Hobject RegEro,RegEroCon,RegSel,RegDil;

	int nBoundOut = 20;//搜索圆环的允许浮动上下限
	int nBoundIn  = 20;
	LowThres = pFinLoc.nLowThres;
	HighThres = pFinLoc.nHighThres;
	int nCenOffset = pFinLoc.nCenOffset;

	smallest_circle(oFinLoc.oInCircle,NULL,NULL,&dInRadii);
	smallest_circle(oFinLoc.oOutCircle,NULL,NULL,&dOutRadii);
	dInRadii=dOutRadii>dInRadii?dInRadii:dOutRadii-10;//防止内环比外环大

	edges_image (Image, &ImaAmp, &ImaDir, "canny", 1.2, "nms", LowThres, HighThres); //默认5,10
	threshold (ImaAmp, &LightReg, 0, 255);
	closing_circle (LightReg, &RegClosing, 5); //11->5
	difference(Image, RegClosing, &RegDiff);
	connection (RegDiff, &RegDiffCon);
	select_shape (RegDiffCon, &regSelect,  HTuple("width").Concat("height"), "and",
		HTuple(2*dInRadii-nBoundIn).Concat(2*dInRadii-nBoundIn),
		HTuple(2*dOutRadii+nBoundOut).Concat(2*dOutRadii+nBoundOut));
	select_shape (regSelect, &regSelect, "roundness", "and", 0.6, 1);
	count_obj (regSelect, &nNum);
	if (nNum < 1)
	{
		return rtnInfo;
	}
	else if (nNum == 1) //num==1,口面和瓶底连在一起
	{
		copy_obj (regSelect, &RegMouth, 1, -1);
	}
	else
	{
		sort_region (regSelect, &SortedRegions, "first_point", "true", "row");
		select_obj (SortedRegions, &RegMouth, 1);
	}
	closing_circle(RegMouth, &RegMouthClo, 35);
	fill_up(RegMouthClo, &RegFillup);
	dOpenRadius = dInRadii-50;
	dOpenRadius>50?dOpenRadius:50;
	opening_circle(RegFillup, &RegOpening, dOpenRadius);
	area_center(RegOpening, &area, NULL, NULL);
	if (area == 0)
	{
		copy_obj (RegFillup, &RegOpening, 1, -1); //RegMouth->RegFillup
	}
	// 排除口外环边缘凸包
	erosion_circle (RegOpening, &RegEro, 5); //10->5
	connection (RegEro, &RegEroCon);
	select_shape_std(RegEroCon, &RegSel, "max_area", 70);
	dilation_circle (RegSel, &RegDil, 5); //10->5
	shape_trans (RegDil, &RegMouthOutCir, "outer_circle");
	smallest_circle (RegMouthOutCir, &Row, &Column, &Radius);
	Low = dOutRadii*0.8;
	High = dOutRadii*1.2;
	if (Radius < Low || Radius > High) //定位到的外环直径过大或过小
	{
		return rtnInfo;
	}
	// 2017.6---张裕判断瓶子放反
	double RowOffset,ColOffset;
	RowOffset = abs(Row-m_nHeight/2);
	ColOffset = abs(Column-m_nWidth/2);
	if (RowOffset > nCenOffset || ColOffset > nCenOffset) //定位到的外环中心与图像中心偏移过大
	{
		rtnInfo.strEx = QObject::tr("The outer ring center is shifted too far from the image center!");
		return rtnInfo;
	}

	oFinLoc.Row = Row;
	oFinLoc.Col = Column;
	oFinLoc.Radius = Radius;

	gen_circle(&oFinLoc.oInCircle,Row,Column,dInRadii);
	gen_circle(&oFinLoc.oOutCircle,Row,Column,dOutRadii);
	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}

//*功能：瓶底定位
RtnInfo CCheck::fnFindPosBase(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pBaseLoc pBaseLoc = para.value<s_pBaseLoc>();
	s_oBaseLoc oBaseLoc = shape.value<s_oBaseLoc>();

	switch(pBaseLoc.nMethodIdx)
	{
	case 0:
		rtnInfo = findPosBaseSeg(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 1:
		rtnInfo = findPosBaseEdge(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 2:
		rtnInfo = findPosBaseBelt(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 3:
		rtnInfo = findPosBaseCircle(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 4:
		rtnInfo = findPosBaseBeltEx(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 5:
		rtnInfo = findPosBaseEdgeMode(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 6:
		rtnInfo = findPosBaseRing(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 7:
		//应力综合定位
		if (normalOri.Row==0 || normalOri.Col==0)
		{
			rtnInfo.nType = ERROR_LOCATEFAIL;
			rtnInfo.strEx = QObject::tr("Normal image locates failure, stress image can not locate!");
		}
		else
		{
			float fscale;//正常与应力图像缩放比例 
			if (0 == oBaseLoc.dradius1 || 0 == oBaseLoc.dradius2)
			{
				fscale = 1;
			}
			else
			{
				fscale = oBaseLoc.dradius2/oBaseLoc.dradius1;
			}
			oBaseLoc.Row = oBaseLoc.drow2 + (normalOri.Row-oBaseLoc.drow1)*fscale;
			oBaseLoc.Col = oBaseLoc.dcol2 + (normalOri.Col-oBaseLoc.dcol1)*fscale;
			oBaseLoc.Radius = normalOri.Radius*fscale;
		}
		break;
	case 8:
		rtnInfo = findPosBaseGray(imgSrc,oBaseLoc,pBaseLoc);
		break;
	case 9:
		rtnInfo = findPosBaseBeltShape(imgSrc,oBaseLoc,pBaseLoc);
		break;
	default:
		break;
	}
	shape.setValue(oBaseLoc);

	if (rtnInfo.nType>0)
	{
		if (pBaseLoc.bIgnore)
		{
			currentOri = modelOri;
			rtnInfo.nType = GOOD_BOTTLE;
		}
		else
		{
			gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		}
	}
	else
	{
		currentOri.Row = oBaseLoc.Row;
		currentOri.Col = oBaseLoc.Col;
		//2014.8.7 之前不传递半径信息，为用于更新瓶底应力定位半径，从原点角度中传递半径
		//2014.9.19 修改角度影响瓶底干扰区域变换，添加半径信息
		currentOri.Radius = oBaseLoc.Radius;
		currentOri.Angle = oBaseLoc.Angle;
	}
	return rtnInfo;
}
//*功能：瓶底分割定位
RtnInfo CCheck::findPosBaseSeg(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hobject regThresh,regCenter,regCircu,XldCenter;
	Hobject ImageReduce, ImageOpen;
	double dGrayMean,Row,Col,Radius;
	Hlong nNum;
	float fSegRatio = pBaseLoc.fSegRatio;

	intensity(oBaseLoc.oCentReg,Image,&dGrayMean,NULL);
	fast_threshold(Image,&regThresh,dGrayMean*fSegRatio,255,20);
	closing_circle(regThresh,&regThresh,1.5);
	connection(regThresh,&regThresh);
	select_shape(regThresh,&regThresh,"area","and",1000,99999999);
	count_obj(regThresh,&nNum);
	if (nNum == 0)
	{		
		return rtnInfo;
	}
	select_shape_std(regThresh,&regCenter,"max_area",70);
	fill_up(regCenter,&regCenter);
	closing_circle(regCenter,&regCenter,35.5);
	fill_up(regCenter, &regCenter);
	reduce_domain(Image, regCenter, &ImageReduce);
	gray_opening_shape(ImageReduce, &ImageOpen, 31, 31, "octagon");
	intensity(oBaseLoc.oCentReg, ImageOpen, &dGrayMean, NULL);
	fast_threshold(ImageOpen, &regCircu, dGrayMean*fSegRatio, 255, 20);
	connection(regCircu, &regCircu);
	fill_up(regCircu, &regCircu);

	//2014.1.7灌装线定位失败
	select_shape(regCircu, &regCircu, "area", "and", 1000, 99999999);
	opening_circle(regCircu, &regCircu, 60.5);//灌装线瓶底分辨率低，由110.5-》60.5
	closing_circle(regCircu, &regCircu, 35.5);
	connection(regCircu, &regCircu);
	select_shape(regCircu, &regCircu, "circularity", "and", 0.3, 1);//灌装线瓶底分辨率低，由0.5-》0.3
	count_obj(regCircu, &nNum);
	if (nNum == 0)
	{
		return rtnInfo;
	}

	select_shape_std(regCircu, &regCircu, "max_area", 70);
	gen_contour_region_xld(regCircu, &XldCenter, "border");
	fit_circle_contour_xld (XldCenter, "algebraic", -1, 0, 0, 3, 2, &Row, &Col, &Radius, NULL, NULL, NULL);

	oBaseLoc.Row = Row;
	oBaseLoc.Col = Col;
	oBaseLoc.Radius = Radius;	

	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}

//*功能：瓶底边缘定位
RtnInfo CCheck::findPosBaseEdge(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hobject objOuter,objInner,XldCenter;
	double Row,Col,Radius;
	Hlong nNum;

	sobel_amp(Image,&objOuter,"sum_abs",3);
	threshold(objOuter,&objOuter,6,255);		
	connection(objOuter,&objOuter);
	select_shape(objOuter,&objOuter,"area","and",150,99999999);
	closing_circle(objOuter,&objOuter,28.5);		
	select_shape(objOuter,&objOuter,HTuple("width").Concat("height"),"and",HTuple(m_nWidth/4).Concat(m_nHeight/4),HTuple(99999).Concat(99999));
	count_obj(objOuter,&nNum);
	if (nNum == 0)
	{
		return rtnInfo;
	}

	union1(objOuter,&objOuter);				
	fill_up(objOuter,&objInner);
	closing_rectangle1(objInner,&objInner,m_nWidth,m_nHeight);
	shape_trans(objInner,&objInner,"convex");
	//如果定位不准，可注释此段
	difference(objInner,objOuter,&objInner);
	connection(objInner,&objInner);
	select_shape_std(objInner,&objInner,"max_area",70);	
	closing_circle(objInner,&objInner,35.5);
	opening_circle(objInner,&objInner,35.5);

	//2013.9.9 nanjc 防止开运算产生空对象，增加面积判断
	select_shape(objInner,&objInner, "area", "and", 1, 99999999);
	count_obj(objInner, &nNum);
	if (nNum == 0)
	{
		return rtnInfo;
	}

	//2018.1 mj 开运算后可能生成多个对象
	connection (objInner, &objInner);
	select_shape_std(objInner,&objInner,"max_area",70);
	//如果定位不准，可注释此段
	gen_contour_region_xld(objInner, &XldCenter, "border");
	fit_circle_contour_xld (XldCenter, "algebraic", -1, 0, 0, 3, 2, &Row, &Col, &Radius, NULL, NULL, NULL);
	oBaseLoc.Row = Row;
	oBaseLoc.Col = Col;
	oBaseLoc.Radius = Radius;

	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;

}

////*功能：瓶底边缘定位--测试时间(抽检机查找轮廓)-待修改
//RtnInfo CCheck::findPosBaseEdge(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
//{
//	RtnInfo rtnInfo;
//	rtnInfo.nType = ERROR_LOCATEFAIL;
//	Hobject EdgeAmplitude,BrightRegion,RegCon,SelRegions,SortedRegions;
//	Hobject ObjSel,RegionDil,ImgReduced,Border,SelectedContours,FinalContour;
//	HTuple Length,Indices,temp;
//
//	    sobel_amp (Image, &EdgeAmplitude, "sum_abs", 3);
//		threshold (EdgeAmplitude, &BrightRegion, 20, 255);
//		connection (BrightRegion, &RegCon);
//		select_shape (RegCon, &SelRegions, "area", "and", 1000, 9999999);
//		sort_region (SelRegions, &SortedRegions, "first_point", "true", "row");
//		select_obj (SortedRegions, &ObjSel, 1);
//		dilation_circle (ObjSel, &RegionDil, 25);
//		reduce_domain (Image, RegionDil, &ImgReduced);
//		edges_sub_pix (ImgReduced, &Border, "canny", 3, 10, 20);
//		select_contours_xld (Border, &SelectedContours, "contour_length", 200, 99999, -0.5, 0.5);
//		length_xld (SelectedContours, &Length);
//		tuple_sort_index (Length, &Indices);
//		temp = Indices[Indices.Num()-1];
//		select_obj (SelectedContours, &FinalContour, temp+1);
//
//	rtnInfo.nType = GOOD_BOTTLE;
//	return rtnInfo;
//
//}

//*功能：瓶底边缘定位(模号)
RtnInfo CCheck::findPosBaseEdgeMode(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hobject regThresh,regValid,regTemp;
	Hobject xldSeg,xldSel;
	Hobject imgReduce,imgSobel;
	HTuple tpLength,tpIndices;
	Hlong nNum;
	double Row,Col,Radius;
	int i;
	double dLocRadius;
	smallest_circle(oBaseLoc.oModeNOEdge,NULL,NULL,&dLocRadius);
	threshold (Image, &regThresh, 50, 255);
	fill_up (regThresh, &regTemp);
	erosion_circle (regTemp, &regTemp, 13.5);
	connection (regTemp, &regTemp);
	select_shape_std (regTemp, &regValid, "max_area", 70);
	reduce_domain (Image, regValid, &imgReduce);
	/*预处理提速*/
	sobel_amp (imgReduce, &imgSobel, "sum_abs", 3);
	threshold (imgSobel, &regThresh, 20, 255);
	connection (regThresh, &regTemp);
	select_shape (regTemp, &regThresh, HTuple("ra").Concat("rb"), "and", 
		HTuple(dLocRadius/4).Concat(dLocRadius/8), HTuple(9999999).Concat(9999999));
	union1 (regThresh, &regValid);
	//closing_circle (regValid, &regTemp, 3.5);
	//connection(regTemp, &regTemp);
	//select_shape_std (regTemp, &regValid, "max_area", 70);
	//intersection(regValid,regThresh,&regValid);
	//union1 (regValid, &regValid);
	reduce_domain (imgReduce, regValid, &imgReduce);
	/**/
	edges_sub_pix (imgReduce, &xldSeg, "sobel_fast", 1, 20, 40);
	segment_contours_xld (xldSeg, &xldSeg, "lines_circles", 3, 4, 2);
	union_cocircular_contours_xld (xldSeg, &xldSeg, 0.5, 0.1, 0.2, 150, 10, 20, "false", 1);
	select_shape_xld (xldSeg, &xldSeg, "contlength", "and", PI*dLocRadius/4, 9999999);
	length_xld (xldSeg, &tpLength);
	tuple_sort_index (tpLength, &tpIndices);
	tuple_length (tpLength, &nNum);
	if (nNum < 1)
	{
		return rtnInfo;
	}
	bool bRsu = false;
	for (i = nNum-1;i>=0;--i)
	{
		select_obj (xldSeg, &xldSel, tpIndices[i].I()+1);
		fit_circle_contour_xld (xldSel, "algebraic", -1, 0, 0, 3, 2, &Row, &Col, &Radius, NULL, NULL, NULL);
		if(fabs(Radius-dLocRadius)<pBaseLoc.nRadiusOffset)
		{
			bRsu = true;
			break;
		}
	}
	if (!bRsu)
	{
		rtnInfo.strEx = QObject::tr("The larger radius offset cause that failure to locate !");//定位半径偏移较大导致定位失败!
		return rtnInfo;
	}
	oBaseLoc.Row = Row;
	oBaseLoc.Col = Col;
	oBaseLoc.Radius = Radius;

	gen_circle(&oBaseLoc.oModeNOEdge,Row,Col,dLocRadius);
	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}
//*功能：瓶底防滑带定位
RtnInfo CCheck::findPosBaseBelt(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hobject imgOpen,imgCent,imgAmp;
	Hobject regThresh,regCenter,regCenter2,regClosing,regCircu,XldCenter;
	double Row,Col,Radius;
	float fSegRatio = pBaseLoc.fSegRatio;
	int nBeltSpace = pBaseLoc.nBeltSpace;
	double dBeltDia,dGrayMean;
	intensity(oBaseLoc.oCentReg,Image,&dGrayMean,NULL);
	smallest_circle(oBaseLoc.oBeltDia,NULL,NULL,&dBeltDia);
	gray_opening_shape(Image,&imgOpen,31,31,"octagon");	
	fast_threshold(imgOpen,&regThresh,dGrayMean*fSegRatio,255,20);		
	connection(regThresh,&regThresh);	
	select_shape_std(regThresh,&regCenter,"max_area",70);		
	closing_circle(regCenter,&regCenter,35.5);	
	area_center(regCenter,NULL,&Row,&Col);
	gen_circle(&regCenter,Row,Col,dBeltDia*1.3);

	reduce_domain(Image,regCenter,&imgCent);		
	sobel_amp(imgCent,&imgAmp,"sum_abs",3);
	threshold(imgAmp,&regCircu,10,255);

	closing_circle(regCircu,&regClosing,nBeltSpace*0.65);
	difference(regCenter,regClosing,&regCircu);		

	erosion_circle(regCircu,&regCircu,3.5);
	connection(regCircu,&regCenter2);
	select_shape_std(regCenter2,&regCenter2,"max_area",70);
	area_center(regCenter2,NULL,&Row,&Col);
	gen_circle(&regCenter2,Row,Col,dBeltDia*1.1);
	intersection(regCircu,regCenter2,&regCircu);
	closing_circle(regCircu,&regCircu,dBeltDia*0.03);
	connection(regCircu,&regCircu);
	select_shape_std(regCircu,&regCircu,"max_area",70);


	Hlong nRegArea,nNum;
	area_center(regCircu,&nRegArea,NULL,NULL);
	if (nRegArea < 10)
	{
		rtnInfo.strEx = QObject::tr("Note whether slip band spacing is too large!");
		return rtnInfo;//20130508 防止异常
	}

	closing_circle(regCircu,&regCircu,35);
	opening_circle(regCircu,&regCircu,35);
	connection(regCircu,&regCircu);
	//2013.9.9 nanjc 开运算产生空对象，拟合圆时奔溃,增加面积判断
	select_shape(regCircu,&regCircu, "area", "and", 1, 99999999);
	count_obj(regCircu, &nNum);
	if (nNum == 0)
	{
		rtnInfo.strEx = QObject::tr("Note whether slip band region is too small!");
		return rtnInfo;
	}

	select_shape_std(regCircu,&regCircu,"max_area",70);
	gen_contour_region_xld(regCircu,&XldCenter,"border");
	fit_circle_contour_xld(XldCenter, "algebraic", -1, 0, 0, 3, 2, &Row, &Col, &Radius, NULL, NULL, NULL);

	oBaseLoc.Row = Row;
	oBaseLoc.Col = Col;
	oBaseLoc.Radius = Radius;

	gen_circle(&oBaseLoc.oBeltDia,Row,Col,dBeltDia);

	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;

}
////防滑块定位(形状)
//RtnInfo findPosBaseBelt_Shape(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
//{
//	RtnInfo rtnInfo;
//
//	return rtnInfo;
//}
//*功能：瓶底防滑块定位
RtnInfo CCheck::findPosBaseBeltEx(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	int debug = 0;
	RtnInfo rtnInfo = findPosBaseSeg(Image,oBaseLoc,pBaseLoc);
	rtnInfo.nType = ERROR_LOCATEFAIL;
	if (rtnInfo.nType != GOOD_BOTTLE)
	{
		return rtnInfo;
	}
	int nBeltWidth = pBaseLoc.nBeltWidth;
	int nBeltHeight = pBaseLoc.nBeltHeight;
	int nBeltEdge = pBaseLoc.nBeltEdge;
	double dBeltDia;
	
	smallest_circle(oBaseLoc.oBeltDia,NULL,NULL,&dBeltDia);	
	int iPolarImageWidth = dBeltDia*2*PI;
	int iPolarImageHeight = dBeltDia*0.8;
	Hobject PolarTransImage,ImageMean;
	Hobject regBeltBand,regBeltBit;
	Hobject regThreshold,regDom,regPolarTrans,regLine,regTemp;
	Hobject regControversyCon,regControversyOpen,regControversyUnion,regControversyClose;
	Hobject regSel1,regSel2,regSelOpen,regSelCon;
	Hlong nNum,nNum1,nNum2,nNum3;
	int i,j;
	Hlong col1,col2;
	HTuple tRows,tCols;
	//1、极坐标变化后提取区域
	polar_trans_image_ext (Image, &PolarTransImage,oBaseLoc.Row, oBaseLoc.Col, 
		0, 2*PI, dBeltDia*0.7, dBeltDia*1.5, iPolarImageWidth, iPolarImageHeight, "nearest_neighbor");
	mean_image (PolarTransImage, &ImageMean, 31, 1);
	dyn_threshold (PolarTransImage, ImageMean, &regThreshold, nBeltEdge, "dark");  //2017.3：开放对比度，默认值为5
	//去掉极坐标变换的边缘区域
	get_domain (Image, &regDom);
	erosion_rectangle1 (regDom, &regDom, 3, 3);
	polar_trans_region (regDom, &regPolarTrans, oBaseLoc.Row, oBaseLoc.Col, 
		0, 2*PI, dBeltDia*0.7, dBeltDia*1.5, iPolarImageWidth, iPolarImageHeight, "nearest_neighbor");
	intersection (regThreshold, regPolarTrans, &regThreshold);
	if(debug)
	{
		write_object(PolarTransImage,".\\PolarTransImage.hobj");
		write_object(regPolarTrans,".\\regPolarTrans.hobj");
	}
	//2、提取防滑块区域
	closing_circle (regThreshold, &regTemp, 1.5);
	opening_circle (regTemp, &regTemp, 1.5);
	closing_rectangle1 (regTemp, &regTemp, 2, 7);
	opening_rectangle1 (regTemp, &regBeltBit, 2, 3);

	//3、提取防滑带有效区域
	//排除小区域干扰和上下边缘凸起干扰
	closing_rectangle1 (regBeltBit, &regTemp, 60, 1);
	connection (regTemp, &regTemp);
	select_shape(regTemp, &regControversyCon, HTuple("width").Concat("height"), "and",
		HTuple(iPolarImageWidth/3.0).Concat(7),HTuple(99999).Concat(99999));

	/*****修改开运算方法 防止起将防滑带有效区域断开*****/
	count_obj (regControversyCon, &nNum);
	if(nNum<1)
	{
		rtnInfo.strEx = QObject::tr("Note whether slip band region is too small!");
		return rtnInfo;
	}
	gen_empty_obj (&regControversyOpen);
	for (i = 1;i<nNum+1;++i)
	{
		select_obj (regControversyCon, &regSel1, i);
		smallest_rectangle1 (regSel1, NULL,&col1,NULL,&col2);
		opening_rectangle1 (regSel1, &regSelOpen, 80, 5);
		concat_obj (regSelOpen, regControversyOpen, &regControversyOpen);
		connection (regSelOpen, &regTemp);
		select_shape (regTemp, &regTemp, "width", "and", (col2-col1)*0.9 ,99999);
		count_obj (regTemp, &nNum1);
		if(nNum1<1)
		{
			difference (regSel1, regSelOpen, &regTemp);
			connection (regTemp, &regSelCon);
			count_obj (regSelCon, &nNum2);
			for (j = 1;j<nNum2+1;++j)
			{
				select_obj (regSelCon, &regSel2, j);
				difference (regSel1, regSel2, &regTemp);
				connection (regTemp, &regTemp);
				select_shape (regTemp, &regTemp, "width", "and", (col2-col1)*0.9 ,99999);
				count_obj (regTemp, &nNum3);
				if(nNum3<1)
					concat_obj (regSel2, regControversyOpen, &regControversyOpen);
			}
		}
	}					
	union1 (regControversyOpen, &regControversyOpen);
	/*****修改开运算方法 防止起将防滑带有效区域断开*****/

	connection (regControversyOpen, &regTemp);
	select_shape(regTemp, &regBeltBand, HTuple("width").Concat("height"), "and",
		HTuple(iPolarImageWidth/3.0).Concat(7),HTuple(99999).Concat(99999));
	count_obj (regBeltBand, &nNum);
	if(nNum<1)
	{
		rtnInfo.strEx = QObject::tr("Note whether slip band region is too small!");
		return rtnInfo;
	}
	/*	copy_obj(regBeltBand,&rtnInfo.regError,1,-1);
	rtnInfo.nType = 1;
	return rtnInfo;	*/		
	//排除下侧边缘码干扰(取上侧区域，排除防滑带以外的边缘干扰)
	union1 (regBeltBand, &regControversyUnion);
	closing_rectangle1 (regControversyUnion, &regControversyClose, 1, iPolarImageHeight);
	difference (regControversyClose, regControversyUnion, &regTemp);
	closing_rectangle1 (regTemp, &regTemp, 10, 1);
	connection (regTemp, &regTemp);
	select_shape(regTemp, &regTemp, HTuple("width").Concat("area"), "and",
		HTuple(100).Concat(1000),HTuple(99999).Concat(99999999));
	if(debug)
	{
		write_object(regTemp,".\\regTemp.hobj");
	}
	count_obj (regTemp, &nNum);
	if(nNum>0)
	{
		gen_region_line (&regLine, iPolarImageHeight-1, 0, iPolarImageHeight-1, iPolarImageWidth);
		union2 (regTemp, regLine, &regTemp);
		closing_rectangle1 (regTemp, &regTemp, 1, iPolarImageHeight);
		difference (regBeltBand, regTemp, &regTemp);
		connection (regTemp, &regTemp);
		select_shape (regTemp, &regBeltBand, HTuple("width").Concat("height"), "and",
			HTuple(iPolarImageWidth/3.0).Concat(7),HTuple(99999).Concat(99999));

		/*****修改开运算方法 防止起将防滑带有效区域断开*****/
		count_obj (regBeltBand, &nNum);
		if(nNum<1)
		{
			rtnInfo.strEx = QObject::tr("Note whether slip band region is too small!");
			return rtnInfo;
		}
		gen_empty_obj (&regControversyOpen);
		for (i = 1;i<nNum+1;++i)
		{
			select_obj (regBeltBand, &regSel1, i);
			smallest_rectangle1 (regSel1, NULL,&col1,NULL,&col2);
			opening_rectangle1 (regSel1, &regSelOpen, 80, 1);
			concat_obj (regSelOpen, regControversyOpen, &regControversyOpen);
			connection (regSelOpen, &regTemp);
			select_shape (regTemp, &regTemp, "width", "and", (col2-col1)*0.9 ,99999);
			count_obj (regTemp, &nNum1);
			if(nNum1<1)
			{
				difference (regSel1, regSelOpen, &regTemp);
				connection (regTemp, &regSelCon);
				count_obj (regSelCon, &nNum2);
				for (j = 1;j<nNum2+1;++j)
				{
					select_obj (regSelCon, &regSel2, j);
					difference (regSel1, regSel2, &regTemp);
					connection (regTemp, &regTemp);
					select_shape (regTemp, &regTemp, "width", "and", (col2-col1)*0.9 ,99999);
					count_obj (regTemp, &nNum3);
					if(nNum3<1)
						concat_obj (regSel2, regControversyOpen, &regControversyOpen);
				}
			}
		}					
		union1 (regControversyOpen, &regBeltBand);
		/*****修改开运算方法 防止起将防滑带有效区域断开*****/
	}

	//4、提取防滑块
	intersection (regBeltBand, regBeltBit, &regTemp);
	if(debug)
	{
	    write_object(regBeltBit,".\\regBeltBit.hobj");
		write_object(regTemp,".\\regTemp_1.hobj");
	}
	opening_rectangle1 (regTemp, &regTemp, 2, 1);
	connection (regTemp, &regTemp);
	select_shape (regTemp, &regTemp, "compactness", "and", 1, 5);
	select_shape (regTemp, &regTemp, "height", "and", nBeltHeight*0.3, nBeltHeight*2);
	select_shape (regTemp, &regBeltBit, "width", "and", nBeltWidth*0.3, nBeltWidth*2);
	
	count_obj (regBeltBit, &nNum);
	//总体个数需大于5
	if(nNum<5)
	{
		rtnInfo.strEx = QObject::tr("Note whether slip bit is too less!");
		return rtnInfo;
	}	
	union1 (regBeltBit, &regTemp);
	smallest_rectangle1 (regTemp, NULL,&col1,NULL,&col2); //注：空区域取最小外接矩形时，若参数为数组，正确，若参数为数，则报异常
	//总体区域宽度需大于一半，总体个数需大于5
	if(col2-col1<iPolarImageWidth/4.0)
	{
		rtnInfo.strEx = QObject::tr("Note whether slip bit region is too less!");
		return rtnInfo;
	}

	polar_trans_region_inv (regBeltBit, &regBeltBit, oBaseLoc.Row, oBaseLoc.Col, 
		0, 2*PI, dBeltDia*0.7, dBeltDia*1.5, iPolarImageWidth, dBeltDia*0.8,m_nWidth,m_nHeight, "nearest_neighbor");
	area_center (regBeltBit,NULL, &tRows, &tCols);
	fitCircle(tRows,tCols,oBaseLoc.Row,oBaseLoc.Col,oBaseLoc.Radius);

	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}
//*功能：瓶底防滑块形状定位  201810 new function for rect bottle
RtnInfo CCheck::findPosBaseBeltShape(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	int debug = 0;
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;
	int nBeltWidth = pBaseLoc.nBeltWidth;
	int nBeltHeight = pBaseLoc.nBeltHeight;
	int nBeltEdge = pBaseLoc.nBeltEdge;
	char aLogInfo[MAX_LOGPATH_LENTH];

	Hlong nNum = 0;
	if(false == oBaseLoc.ifGenSp)
	{	
		Hlong isExist;
		file_exists(".\\model_nut.shm", &isExist);
		if(TRUE == isExist)
		{
			read_shape_model( ".\\model_nut.shm",&oBaseLoc.ModelID);
		}
		else
		{
			HTuple trad,trad1;
		
			Hobject RegionBorder1,RegionDilation1,imgReduce,ImageMean,
					regThreshold,imgReduced,ModelImages,ModelRegions;
			Hobject oBeltDiaR;
			float trad0 = 0,trad01 = 0;
			double fRow,fCol,fRow1,fCol1;
			smallest_rectangle1_xld(oBaseLoc.oBeltDia_Rect,&fRow,&fCol,&fRow1,&fCol1);
			gen_rectangle1(&oBeltDiaR,fRow,fCol,fRow1,fCol1);
			
			boundary(oBeltDiaR, &RegionBorder1, "inner");
			dilation_circle(RegionBorder1, &RegionDilation1, 80);
			intersection(oBeltDiaR, RegionDilation1,&RegionDilation1);
			reduce_domain(Image, RegionDilation1, &imgReduce);
			
			mean_image (imgReduce, &ImageMean, 31, 1);
			dyn_threshold (imgReduce, ImageMean, &regThreshold, pBaseLoc.nBeltEdge, "dark");
			connection (regThreshold, &regThreshold);
			select_shape (regThreshold, &regThreshold,"outer_radius", "and", 2, pBaseLoc.nBeltWidth * 2 );//cont length of mould point
			//select_shape (regThreshold, &regThreshold,"ra", "and", 2, pBaseLoc.nBeltWidth * 2 );
			opening_circle(regThreshold, &regThreshold,2);  
			union1(regThreshold,&regThreshold);    
			reduce_domain(imgReduce, regThreshold, &imgReduced);
	    
			tuple_rad(0,&trad);
			tuple_rad(360,&trad1);
			trad0 = float(trad[0].D());
			trad01 = float(trad1[0].D());
			create_shape_model (imgReduced, 5, trad0, trad01, 0,"auto", "use_polarity", 25, 5, &oBaseLoc.ModelID);
			inspect_shape_model (imgReduced, &ModelImages, &ModelRegions, 1, 15);
		}
		//tuple_length(oBaseLoc.ModelID,&nNum); 
		if(oBaseLoc.ModelID < 0 )
		{
			rtnInfo.strEx = QObject::tr("Note whether slip bit region is too less for template!");
			return rtnInfo;
		}
		get_shape_model_contours(&oBaseLoc.ShapeModel, oBaseLoc.ModelID, 1);
		oBaseLoc.ifGenSp = true;
	}
	HTuple trad;
	HTuple hrow,hcol,hangle,hscore;
	HTuple MovementOfObject;
	
	tuple_rad(180,&trad);
	find_shape_model(Image, oBaseLoc.ModelID , -trad, trad, 0.1, 1, 0.3, "interpolation", 0, 0.9, &hrow,&hcol, &hangle, &hscore);
	tuple_length(hscore,&nNum);
	if( nNum < 1)
	{
		rtnInfo.strEx = QObject::tr("Note whether slip bit region is too less!");
		return rtnInfo;
	}
	else
	{
        hom_mat2d_identity(&MovementOfObject);//display
        vector_angle_to_rigid (0, 0, 0, hrow[0], hcol[0],hangle[0], &MovementOfObject);
        affine_trans_contour_xld(oBaseLoc.ShapeModel, &oBaseLoc.FoundModel, MovementOfObject);
        //dev_display (oBaseLoc.FoundModel)
		oBaseLoc.Row = float(hrow[0].D());
		oBaseLoc.Col = float(hcol[0].D());
		oBaseLoc.Angle = float(hangle[0].D());
	}
	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}
//*功能：瓶底灰度定位---2017.12新增
RtnInfo CCheck::findPosBaseGray(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	Hobject EdgeAmplitude,CircleOut,CircleIn,CircleDiff;
	double MinMean,MinJ,MinI,Mean;
	double dRow,dCol,dBeltDia;
	int nMoveOffset = pBaseLoc.nMoveOffset; 
	int nMoveStep = pBaseLoc.nMoveStep;
	MinMean = 999;
	MinJ = 0;
	MinI = 0;

	smallest_circle(oBaseLoc.oBeltDia,&dRow,&dCol,&dBeltDia);
	sobel_amp (Image,&EdgeAmplitude,"sum_abs", 3);

	for (int J= -nMoveOffset; J<=nMoveOffset; J=J+nMoveStep)  //以圆心为中心，上下和左右各移动nMoveOffset个像素，默认20。
	{
		  for (int I= -nMoveOffset; I<=nMoveOffset; I=I+nMoveStep)
		  {
			  gen_circle (&CircleOut, dRow+J, dCol+I, dBeltDia);
			  gen_circle (&CircleIn, dRow+J, dCol+I, dBeltDia-10);
			  difference (CircleOut, CircleIn, &CircleDiff);
			  intensity (CircleDiff, EdgeAmplitude, &Mean, NULL);
			  if (Mean<MinMean)
			  {
				 MinJ = J;
				 MinI = I;
				 MinMean = Mean;  
			  }
		  }
	}

	oBaseLoc.Row = dRow+MinJ;
	oBaseLoc.Col = dCol+MinI;
	oBaseLoc.Radius = dBeltDia;

	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;
}

//*功能：拟合圆(最小二乘法)
void CCheck::fitCircle(const HTuple &tRows,const HTuple &tCols,float & CircleRow,float &CircleCol,float & CircleRadius)
{
	CircleRow = 0;
	CircleCol = 0;
	CircleRadius = 0;
	Hlong N;
	HTuple temp;
	double X1,Y1,X2,Y2,X3,Y3,X1Y1,X1Y2,X2Y1;
	double a,b,c,C,D,E,G,H;
	tuple_length (tRows, &N);
	if (N<3) 
		return;

	tuple_sum (tRows, &X1);
	tuple_sum (tCols, &Y1);
	tuple_mult (tRows, tRows, &temp);
	tuple_sum (temp, &X2);
	tuple_mult (temp, tRows, &temp);
	tuple_sum (temp, &X3);
	tuple_mult (tCols, tCols, &temp);
	tuple_sum (temp, &Y2);
	tuple_mult (temp, tCols, &temp);
	tuple_sum (temp, &Y3);
	tuple_mult (tRows, tCols, &temp);
	tuple_sum (temp, &X1Y1);
	tuple_mult (tRows, tCols, &temp);
	tuple_mult (temp, tCols, &temp);
	tuple_sum (temp, &X1Y2);
	tuple_mult (tRows, tRows, &temp);
	tuple_mult (temp, tCols, &temp);
	tuple_sum (temp, &X2Y1);

	C = N*X2 - X1*X1;   
	D = N*X1Y1 - X1*Y1;   
	E = N*X3 + N*X1Y2 - (X2+Y2)*X1;  
	G = N*Y2 - Y1*Y1;   
	H = N*X2Y1 + N*Y3 - (X2+Y2)*Y1;   
	a = (H*D-E*G)/(C*G-D*D);   
	b = (H*C-E*D)/(D*D-G*C);  
	c = -(a*X1 + b*Y1 + X2 + Y2)/N;    

	CircleRow = a/(-2);    
	CircleCol = b/(-2);    
	CircleRadius = sqrt(a*a+b*b-4*c)/2;  
}
//*功能：瓶底内圈定位（红酒）
RtnInfo CCheck::findPosBaseCircle(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	double fCentDia;
	float fSegRatio = pBaseLoc.fSegRatio;
	double dTolerRange=0.2;//容许内圈浮动范围
	Hlong nNum;
	Hobject regDomain,regRect,regThresh,regMiddle,regTemp,regSel;
	Hobject xldCircle;
	Hobject imgMean;
	Hobject objCent,objSel;
	HTuple tpArea,tpCircul,tpLength,tpIndices;
	double Row,Col,Radius;
	int mMaskScale = 31;
	/*
	smallest_circle(oBaseLoc.oCentReg,NULL,NULL,&fCentDia);	
	gen_empty_obj(&objSel);
	threshold(Image,&objCent,0,fSegRatio*255);
	closing_circle(objCent,&objCent,7);
	connection(objCent,&objCent);
	select_shape(objCent,&objCent,HTuple("height").Concat("width"),"or",
		HTuple(2*fCentDia*(1-dTolerRange)).Concat(2*fCentDia*(1-dTolerRange)),
		HTuple(2*fCentDia*(1+dTolerRange)).Concat(2*fCentDia*(1+dTolerRange)));
	count_obj(objCent,&nNum);
	if (nNum==0)
	{
		rtnInfo.strEx = QObject::tr("The inner ring is not normal or split ratio is not suitable!");
		return rtnInfo;
	}
	else if (nNum==1)
	{
		copy_obj(objCent,&objSel,1,-1);
	}
	else if (nNum>1)
	{
		intensity(objCent,Image,&tpMeanGray,NULL);
		tuple_sort_index(tpMeanGray,&tpIndices);
		select_obj(objCent,&objSel,tpIndices[0].I()+1);
	}	
	smallest_circle(objSel,&Row,&Col,&Radius);	
	*/
	//2015.1.12 根据方圆提供图片，修改定位方法
	get_domain(Image,&regDomain);
	smallest_circle(oBaseLoc.oCentReg,NULL,NULL,&fCentDia);	
	mean_image(Image,&imgMean,mMaskScale,mMaskScale);
	dyn_threshold(Image,imgMean,&regThresh,pBaseLoc.nEdge,"dark");
	if (pBaseLoc.nGray>0)
	{
		threshold(Image,&regTemp,0,pBaseLoc.nGray);
		union2(regThresh,regTemp,&regThresh);
	}
	dilation_circle (regThresh, &regTemp, 1);
	closing_circle (regTemp, &regTemp, 6);
	connection (regTemp, &regTemp);
	//select_shape (regTemp, &regTemp, "area", "and", 100, 999999999);
	//select_shape (regTemp, &regTemp, "rect2_len1", "and", 30, 999999999);
	//select_shape_proto (regTemp, regDomain, &regTemp, "distance_contour", 10, 99999);     
	//return rtnInfo;
	fill_up (regTemp, &regMiddle);
	//select_shape_proto (regMiddle, regDomain, &regThresh, "distance_contour", 10, 99999);     
	//2015.1.13 select_shape_proto耗时长修改为如下方法
	gen_rectangle1(&regRect,11,11,max(m_nHeight-11,12),max(m_nWidth-11,12));
	difference (regMiddle,regRect,&regTemp);
	count_obj (regTemp, &nNum);
	area_center (regTemp, &tpArea, NULL, NULL);
	tuple_find (tpArea, 0, &tpIndices);
	select_obj(regMiddle,&regThresh,tpIndices+1);
	//////////////////////////////////////////////////////////////////////////
	opening_circle (regThresh, &regTemp, fCentDia/2);
	select_shape (regTemp, &regTemp, "circularity", "and", 0.8, 1);
	count_obj (regTemp, &nNum);
	if (nNum>0)
	{
		area_center (regTemp, &tpArea,NULL, NULL);
		tuple_sort_index (tpArea, &tpIndices);
		select_obj (regTemp, &regSel, tpIndices[0].I()+1);
	}
	else
	{
		select_shape(regThresh, &regMiddle, "rect2_len1", "and",fCentDia/3*(1-dTolerRange),fCentDia*(1+dTolerRange));//长度预选
		select_shape(regMiddle, &regTemp, HTuple("rect2_len1").Concat("rect2_len2"), "and",
			HTuple(fCentDia/2*(1-dTolerRange)).Concat(fCentDia/4),HTuple(fCentDia*(1+dTolerRange)).Concat(fCentDia*(1+dTolerRange)));
		count_obj (regTemp, &nNum);
		if (0 == nNum)
		{
			count_obj (regMiddle, &nNum);
			if (0 == nNum)
			{
				//copy_obj(regThresh,&rtnInfo.regError,1,-1);
				return rtnInfo;
			}
			//inner_circle(regMiddle,NULL,NULL,&tpLength);
			//tuple_sort_index (tpLength, &tpIndices);
			//select_obj (regMiddle, &regSel, tpIndices[0].I()+1);
			union1(regMiddle,&regSel);
		}
		else
		{
			smallest_rectangle2 (regTemp, NULL, NULL, NULL, NULL, &tpLength);
			tuple_sort_index (tpLength, &tpIndices);
			tuple_length(tpIndices,&nNum);
			select_obj (regTemp, &regSel, tpIndices[nNum-1].I()+1);
		}
	}
	gen_contour_region_xld (regSel, &xldCircle, "border_holes");
	segment_contours_xld (xldCircle, &xldCircle, "lines_circles", 7, 4, 2);
	union_cocircular_contours_xld (xldCircle, &xldCircle, 0.5, 0.1, 0.2, 30, 10, 10, "true", 1);
	select_contours_xld (xldCircle, &xldCircle, "contour_length", 100, 99999999, 100, 99999999);
	circularity_xld (xldCircle, &tpCircul);
	tuple_sort_index (tpCircul, &tpIndices);
	tuple_length(tpIndices,&nNum);
	if (0 == nNum)
	{
		return rtnInfo;
	}
	if(0 == tpCircul[nNum-1].D())
	{
		return rtnInfo;
	}
	select_obj (xldCircle, &xldCircle, tpIndices[nNum-1].I()+1);
	fit_circle_contour_xld (xldCircle, "algebraic", -1, 0, 0, 3, 2, &Row, &Col, &Radius, NULL, NULL, NULL);
	oBaseLoc.Row = Row;
	oBaseLoc.Col = Col;
	oBaseLoc.Radius = fCentDia;

	gen_circle(&oBaseLoc.oCentReg,Row,Col,fCentDia);

	rtnInfo.nType = GOOD_BOTTLE;
	return rtnInfo;

}
//*功能：瓶底内环定位（红酒）
RtnInfo CCheck::findPosBaseRing(const Hobject &Image,s_oBaseLoc &oBaseLoc,s_pBaseLoc &pBaseLoc)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_LOCATEFAIL;

	double fCentDia;
	float fSegRatio = pBaseLoc.fSegRatio;
	double dTolerRange=0.5;//容许内圈浮动范围
	Hlong nNum;
	Hobject regDomain,regRect,regThresh,regMiddle,regTemp,regSel;
	Hobject xldCircle;
	Hobject imgMean;
	Hobject objCent,objSel;
	HTuple tpArea,tpCircul,tpLength,tpIndices;
	double Row,Col,Radius;
	int mMaskScale = 31;
	smallest_circle(oBaseLoc.oCentReg,NULL,NULL,&fCentDia);	
	get_domain(Image,&regDomain);
	gen_empty_obj(&objSel);
	threshold(Image,&objCent,0,fSegRatio*255);
	closing_circle(objCent,&objCent,7);
	connection(objCent,&objCent);
	select_shape(objCent,&objCent,HTuple("height").Concat("width"),"or",
		HTuple(2*fCentDia).Concat(2*fCentDia),
		HTuple(99999).Concat(99999));
	union1(objCent,&objCent);
	difference(regDomain,objCent,&objCent);
	connection(objCent,&objCent);
	select_shape(objCent,&objCent,HTuple("height").Concat("width"),"or",
		HTuple(2*fCentDia*(1-dTolerRange)).Concat(2*fCentDia*(1-dTolerRange)),
		HTuple(2*fCentDia*(1+dTolerRange)).Concat(2*fCentDia*(1+dTolerRange)));
	select_shape(objCent,&objCent,"circularity","or",0.3,1);
	select_shape_std(objCent,&objCent,"max_area",70);
	count_obj(objCent,&nNum);
	if (nNum==0)
	{
		rtnInfo.strEx = QObject::tr("The inner ring is not normal or split ratio is not suitable!");
		return rtnInfo;
	}
	else
	{
		fill_up(objCent,&objCent);
		gen_contour_region_xld (objCent, &xldCircle, "border_holes");
		fit_circle_contour_xld (xldCircle, "algebraic", -1, 0, 0, 3, 2, &Row, &Col, &Radius, NULL, NULL, NULL);
		oBaseLoc.Row = Row;
		oBaseLoc.Col = Col;
		oBaseLoc.Radius = fCentDia;
		gen_circle(&oBaseLoc.oCentReg,Row,Col,fCentDia);
		rtnInfo.nType = GOOD_BOTTLE;
		return rtnInfo;
	}	
}
//*功能：计算横向尺寸
RtnInfo CCheck::fnHoriSize(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_HORISIZE;
	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	s_pHoriSize pHoriSize = para.value<s_pHoriSize>();
	s_oHoriSize oHoriSize = shape.value<s_oHoriSize>();

	if (!pHoriSize.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double row1,col1,row2,col2;
	Hobject oLineSeg,regPro;
	HTuple rowPt,colPt;	
	smallest_rectangle1_xld(oHoriSize.oSizeRect,&row1,&col1,&row2,&col2);
	if ((row2-row1)<3)
	{
		rtnInfo.strEx=QObject::tr("Height of detection region is too small");
		return rtnInfo;
	}
	//修改原尺寸矩形
	//gen_rectangle1(&oHoriSize.oSizeRect,(row1+row2)/2-15,col1,(row1+row2)/2+15,col2);
	gen_rectangle2_contour_xld(&oHoriSize.oSizeRect,(row1+row2)/2,(col1+col2)/2,0,(col2-col1)/2,15);
	shape.setValue(oHoriSize);

	gen_region_line(&oLineSeg,(row1+row2)/2,col1,(row1+row2)/2,col2);	

	if (findEdgePointDouble(imgSrc,oLineSeg,&rowPt,&colPt,pHoriSize.nEdge)<2)
	{		
		rtnInfo.strEx=QObject::tr("Can not find the boundary points around, please check the position of the rectangle!");
		gen_region_contour_xld(oHoriSize.oSizeRect,&regPro,"filled");
		copy_obj(regPro, &rtnInfo.regError, 1, -1); //注：rtnInfo.regError必须是region类型
		return rtnInfo;
	}
	row1 = rowPt[0];
	col1 = colPt[0];
	row2 = rowPt[1];
	col2 = colPt[1];
	
	oHoriSize.ptLeft = QPoint(col1,row1);
	oHoriSize.ptRight =QPoint(col2,row2);
	pHoriSize.fCurValue = (col2-col1)*pHoriSize.fRuler;
	para.setValue(pHoriSize);
	shape.setValue(oHoriSize);
	if (pHoriSize.fCurValue>pHoriSize.fUpper)
	{
		if (pHoriSize.fCurValue<pHoriSize.fUpper+pHoriSize.fModify)
		{
			pHoriSize.fCurValue = pHoriSize.fUpper;
		}
		else
		{
			rtnInfo.strEx=QObject::tr("Exceed the upper limit");
			gen_region_contour_xld(oHoriSize.oSizeRect,&regPro,"filled");
			copy_obj(regPro, &rtnInfo.regError, 1, -1);
			return rtnInfo;
		}
	}
	if (pHoriSize.fCurValue<pHoriSize.fLower)
	{
		if (pHoriSize.fCurValue>pHoriSize.fLower-pHoriSize.fModify)
		{
			pHoriSize.fCurValue = pHoriSize.fLower;
		}
		else
		{
			rtnInfo.strEx=QObject::tr("Exceed the lower limit");
			gen_region_contour_xld(oHoriSize.oSizeRect,&regPro,"filled");
			copy_obj(regPro, &rtnInfo.regError, 1, -1);
			return rtnInfo;
		}
	}
	//2014.6.18 修正参数
	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：计算纵向尺寸
RtnInfo CCheck::fnVertSize(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_VERTSIZE;
	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	s_pVertSize pVertSize = para.value<s_pVertSize>();
	s_oVertSize oVertSize = shape.value<s_oVertSize>();

	if (!pVertSize.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double row1,col1,row2,col2;
	Hobject oLineSeg,regPro;
	HTuple rowPt,colPt;	
	smallest_rectangle1_xld(oVertSize.oSizeRect,&row1,&col1,&row2,&col2);
	if ((col2-col1)<3)
	{
		rtnInfo.strEx=QObject::tr("Width of detection region is too small");
		return rtnInfo;
	}
	//修改原尺寸矩形
	//gen_rectangle1(&oVertSize.oSizeRect,row1,(col1+col2)/2-15,row2,(col1+col2)/2+15);
	gen_rectangle2_contour_xld(&oVertSize.oSizeRect,(row1+row2)/2,(col1+col2)/2,0,15,(row2-row1)/2);
	shape.setValue(oVertSize);

	gen_region_line(&oLineSeg,row1,(col1+col2)/2,row2,(col1+col2)/2);
	if (findEdgePointDouble(imgSrc,oLineSeg,&rowPt,&colPt,pVertSize.nEdge,T2B)<2)
	{		
		rtnInfo.strEx=QObject::tr("Can not find the boundary points around, please check the position of the rectangle!");
		gen_region_contour_xld(oVertSize.oSizeRect,&regPro,"filled");
		copy_obj(regPro, &rtnInfo.regError, 1, -1);
		return rtnInfo;
	}
	row1 = rowPt[0];
	col1 = colPt[0];
	row2 = rowPt[1];
	col2 = colPt[1];
	oVertSize.ptLeft = QPoint(col1,row1);
	oVertSize.ptRight =QPoint(col2,row2);
	pVertSize.fCurValue = (row2-row1)*pVertSize.fRuler;
	shape.setValue(oVertSize);
	bool bRsu = true;
	if (pVertSize.fCurValue>pVertSize.fUpper)
	{
		if (pVertSize.fCurValue<pVertSize.fUpper+pVertSize.fModify)
		{
			pVertSize.fCurValue = pVertSize.fUpper;
		}
		else
		{
			bRsu = false;
			rtnInfo.strEx=QObject::tr("Exceed the upper limit");
			gen_region_contour_xld(oVertSize.oSizeRect,&regPro,"filled");
			copy_obj(regPro, &rtnInfo.regError, 1, -1);
		}
	}
	if (pVertSize.fCurValue<pVertSize.fLower)
	{
		if (pVertSize.fCurValue>pVertSize.fLower-pVertSize.fModify)
		{
			pVertSize.fCurValue = pVertSize.fLower;
		}
		else
		{
			bRsu = false;
			rtnInfo.strEx=QObject::tr("Exceed the lower limit");
			gen_region_contour_xld(oVertSize.oSizeRect,&regPro,"filled");
			copy_obj(regPro, &rtnInfo.regError, 1, -1);
		}
	}
	para.setValue(pVertSize);
	if (bRsu)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
	}
	return rtnInfo;
}
//*瓶全高
RtnInfo CCheck::fnFullHeight(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_VERTSIZE;
	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	s_pFullHeight pFullHeight = para.value<s_pFullHeight>();
	s_oFullHeight oFullHeight = shape.value<s_oFullHeight>();

	if(!pFullHeight.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double row1,col1,row2,col2;
	Hobject oLineSeg,regPro;
	HTuple rowPt,colPt;	
	smallest_rectangle1_xld(oFullHeight.oSizeRect,&row1,&col1,&row2,&col2);
	if ((col2-col1)<3)
	{
		rtnInfo.strEx=QObject::tr("Width of detection region is too small");
		return rtnInfo;
	}
	//修改原尺寸矩形
	//gen_rectangle1(&oVertSize.oSizeRect,row1,(col1+col2)/2-15,row2,(col1+col2)/2+15);
	gen_rectangle2_contour_xld(&oFullHeight.oSizeRect,(row1+row2)/2,(col1+col2)/2,0,15,(row2-row1)/2);
	shape.setValue(oFullHeight);

	gen_region_line(&oLineSeg,row1,(col1+col2)/2,row2,(col1+col2)/2);
	if (findEdgePointSingle(imgSrc,oLineSeg,&rowPt,&colPt,pFullHeight.nEdge,T2B)<1)
	{		
		rtnInfo.strEx=QObject::tr("Can not find the boundary points around, please check the position of the rectangle!");
		gen_region_contour_xld(oFullHeight.oSizeRect,&regPro,"filled");
		copy_obj(regPro, &rtnInfo.regError, 1, -1);
		return rtnInfo;
	}
	row2 = row2;
	col2 = (col1+col2)/2;
	row1 = rowPt[0];
	col1 = colPt[0];
	oFullHeight.ptLeft = QPoint(col1,row1);
	oFullHeight.ptRight =QPoint(col2,row2);
	pFullHeight.fCurValue = (row2-row1)*pFullHeight.fRuler;
	shape.setValue(oFullHeight);
	bool bRsu = true;
	if (pFullHeight.fCurValue>pFullHeight.fUpper)
	{
		if (pFullHeight.fCurValue<pFullHeight.fUpper+pFullHeight.fModify)
		{
			pFullHeight.fCurValue = pFullHeight.fUpper;
		}
		else
		{
			bRsu = false;
			rtnInfo.strEx=QObject::tr("Exceed the upper limit");
			gen_region_contour_xld(oFullHeight.oSizeRect,&regPro,"filled");
			copy_obj(regPro, &rtnInfo.regError, 1, -1);
		}
	}
	if (pFullHeight.fCurValue<pFullHeight.fLower)
	{
		if (pFullHeight.fCurValue>pFullHeight.fLower-pFullHeight.fModify)
		{
			pFullHeight.fCurValue = pFullHeight.fLower;
		}
		else
		{
			bRsu = false;
			rtnInfo.strEx=QObject::tr("Exceed the lower limit");
			gen_region_contour_xld(oFullHeight.oSizeRect,&regPro,"filled");
			copy_obj(regPro, &rtnInfo.regError, 1, -1);
		}
	}
	para.setValue(pFullHeight);
	if (bRsu)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
	}
	return rtnInfo;
}
//*功能：计算圆形尺寸（外径 & 椭圆度）
RtnInfo CCheck::fnCircleSize(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_oCirSize oCirSize = shape.value<s_oCirSize>();
	s_pCirSize pCirSize = para.value<s_pCirSize>();
	if (!pCirSize.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	if (pCirSize.bDia || pCirSize.bOvality)
	{
		Hobject imgMean;
		Hobject regOutCir,regInCir,regCheck,regDyn,regCon,regTemp,regCircle;
		double Row,Column,dRadius,dRad1,dRad2;
		Hlong lNum;
		smallest_circle(oCirSize.oCircle,&Row,&Column,&dRadius);
		pCirSize.fDiaCurValue = 2*dRadius*pCirSize.fDiaRuler;
		pCirSize.fOvalCurValue = 0;
		mean_image(imgSrc,&imgMean,31,31);
		dyn_threshold(imgSrc,imgMean,&regDyn,pCirSize.nEdge,"light");
		closing_circle(regDyn,&regDyn,3.5);
		connection(regDyn,&regCon);
		gen_circle(&regOutCir,Row,Column,dRadius+35);
		gen_circle(&regInCir,Row,Column,dRadius-35);
		difference(regOutCir,regInCir,&regCheck);
		intersection(regCon,regCheck,&regTemp);
		count_obj(regTemp,&lNum);
		if (lNum>0)	
		{
			select_shape_std(regTemp,&regTemp,"max_area",70);
			shape_trans(regTemp,&regTemp,"convex");
			fill_up(regTemp,&regTemp);
			gen_contour_region_xld(regTemp,&regCircle,"center");
			select_shape_xld(regCircle,&regCircle,HTuple("contlength").Concat("circularity"),
				"and", HTuple(6.28*dRadius*0.8).Concat(0.8), HTuple(99999).Concat(1));
			count_obj(regCircle,&lNum);
			if (lNum>0)
			{
				fit_ellipse_contour_xld(regCircle,"fitzgibbon",-1,0,0,200,3,2,NULL,NULL,NULL,&dRad1,&dRad2,NULL,NULL,NULL);
				pCirSize.fDiaCurValue = (dRad1+dRad2)*pCirSize.fDiaRuler;
				pCirSize.fOvalCurValue = fabs(dRad1-dRad2)*pCirSize.fDiaRuler;
				if (pCirSize.bDia)
				{
					if (pCirSize.fDiaCurValue>pCirSize.fDiaUpper)
					{
						if (pCirSize.fDiaCurValue<pCirSize.fDiaUpper+pCirSize.fDiaModify)
						{
							pCirSize.fDiaCurValue = pCirSize.fDiaUpper;
						}
						else
						{
							gen_region_contour_xld(regCircle,&rtnInfo.regError,"filled");
							rtnInfo.nType = ERROR_CIRCLE_DIA;
							rtnInfo.strEx=QObject::tr("Exceed the upper limit");
							para.setValue(pCirSize);
							return rtnInfo;
						}
					}
					if (pCirSize.fDiaCurValue<pCirSize.fDiaLower)
					{
						if (pCirSize.fDiaCurValue>pCirSize.fDiaLower-pCirSize.fDiaModify)
						{
							pCirSize.fDiaCurValue = pCirSize.fDiaLower;
						}
						else
						{
							gen_region_contour_xld(regCircle,&rtnInfo.regError,"filled");
							rtnInfo.nType = ERROR_CIRCLE_DIA;
							rtnInfo.strEx=QObject::tr("Exceed the lower limit");
							para.setValue(pCirSize);
							return rtnInfo;
						}
					}
				}

				if (pCirSize.bOvality)
				{
					if (pCirSize.fOvalCurValue>pCirSize.fOvality)
					{
						gen_region_contour_xld(regCircle,&rtnInfo.regError,"filled");
						rtnInfo.nType = ERROR_CIRCLE_OVALITY;
						rtnInfo.strEx=QObject::tr("Error ovality");
						para.setValue(pCirSize);
						return rtnInfo;
					}
				}
			}
		}
	}
	para.setValue(pCirSize);
	return rtnInfo;
}
//*功能：计算歪脖度
RtnInfo CCheck::fnBentNeck(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_BENTNECK;
	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	s_pBentNeck pBentNeck = para.value<s_pBentNeck>();
	s_oBentNeck oBentNeck = shape.value<s_oBentNeck>();

	if (!pBentNeck.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double row1,col1,row2,col2;
	Hobject oLineSeg;
	HTuple rowPt,colPt;	
	pBentNeck.nCurValue = 0;
	smallest_rectangle1_xld(oBentNeck.oFinRect,&row1,&col1,&row2,&col2);
	if ((row2-row1)<3)
	{
		rtnInfo.strEx=QObject::tr("Height of detection region is too small");
		return rtnInfo;
	}
	gen_region_line(&oLineSeg,(row1+row2)/2,col1,(row1+row2)/2,col2);
	if (findEdgePointDouble(imgSrc,oLineSeg,&rowPt,&colPt,10)<2)
	{		
		rtnInfo.strEx=QObject::tr("Can not find the edge of the bottle mouth, please check the position of the rectangle!");
		gen_region_contour_xld(oBentNeck.oFinRect,&rtnInfo.regError,"filled");
		return rtnInfo;
	}
	row1 = rowPt[0];
	col1 = colPt[0];
	row2 = rowPt[1];
	col2 = colPt[1];

	pBentNeck.nCurValue =abs((col1+col2)/2-currentOri.Col);
	para.setValue(pBentNeck);
	if (pBentNeck.nCurValue>pBentNeck.nBentNeck)
	{		
		gen_region_contour_xld(oBentNeck.oFinRect,&rtnInfo.regError,"filled");
		return rtnInfo;
	}

	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：计算垂直度
RtnInfo CCheck::fnVertAng(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_VERTANG;
	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	s_pVertAng pVertAng = para.value<s_pVertAng>();

	if (!pVertAng.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double dCurAng = currentOri.Angle;
	tuple_deg(dCurAng,&dCurAng);
	if(dCurAng<0)
		dCurAng+=180;

	pVertAng.fCurValue = dCurAng-pVertAng.fRuler;
	para.setValue(pVertAng);
	if (fabs(pVertAng.fCurValue)>pVertAng.fVertAng)
	{
		return rtnInfo;
	}

	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：瓶身普通区域
RtnInfo CCheck::fnSGenReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSGenReg pSGenReg = para.value<s_pSGenReg>();
	s_oSGenReg oSGenReg = shape.value<s_oSGenReg>();
	if (!pSGenReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	//// 2017.5---薄皮气泡变量
	//Hobject regBubbles,ImaAmp,ImaDir,RegDark,RegDarkClo,RegCon,BubbleCan;
	//Hobject SelObj,RegionZoom,RegionMoved,RegTrans,RegDil,RegInter;
	//Hobject RegSel,OutReg,RegOther,RegOtherCon,OutRegCir;
	//Hobject OutRegFillup,RegOpening,RegDiff;
	//double Mean1,Mean2,RowCen,ColCen,RowCenZoom,ColCenZoom,CenRow,CenCol;
	//Hlong count,IsInside;
	//HTuple area,areaMax,ind,areaFillup,areaOpening;

	Hlong nNum/*,nArea*/;
	int i;
	double Ra,Rb,Comp,Conv;
	Hobject imgReduce,imgMean;
	Hobject regThresh,regBinThresh,regTemp,regBlob,regLigStripe,regDisturb,regDiff;
	Hobject objSpecial;//完全不检、典型缺陷及条纹区域
	gen_empty_obj(&objSpecial);
	if (pSGenReg.nShapeType == 0)
	{
		gen_region_contour_xld(oSGenReg.oCheckRegion,&oSGenReg.oValidRegion,"filled");
	}
	else
	{
		gen_region_contour_xld(oSGenReg.oCheckRegion_Rect,&oSGenReg.oValidRegion,"filled");
	}

	clip_region(oSGenReg.oValidRegion,&oSGenReg.oValidRegion,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(oSGenReg.oValidRegion,&oSGenReg.oValidRegion,"area","and",100,99999999);
	count_obj(oSGenReg.oValidRegion,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}
	//检测区域预处理
	if (pSGenReg.bGenRoi)
	{
		s_ROIPara roiPara;
		roiPara.fRoiRatio = pSGenReg.fRoiRatio;
		roiPara.nClosingWH = pSGenReg.nClosingWH;
		roiPara.nGapWH = pSGenReg.nGapWH;
		rtnInfo = genValidROI(imgSrc,roiPara,oSGenReg.oValidRegion,&oSGenReg.oValidRegion);
		if (rtnInfo.nType>0)
		{
			return rtnInfo;
		}
	}
	shape.setValue(oSGenReg);

	//检测一般缺陷
	reduce_domain(imgSrc,oSGenReg.oValidRegion,&imgReduce);
	Hlong Row1, Col1, Row2, Col2;
	int nMaskSizeWidth,nMaskSizeHeight;
	smallest_rectangle1(oSGenReg.oValidRegion, &Row1, &Col1, &Row2, &Col2);	
	nMaskSizeWidth = min((Col2-Col1)/3,pSGenReg.nMaskSize);
	nMaskSizeWidth = max(1,nMaskSizeWidth);
	nMaskSizeHeight = min((Row2-Row1)/3,pSGenReg.nMaskSize);
	nMaskSizeHeight = max(1,nMaskSizeHeight);
	mean_image(imgReduce, &imgMean, nMaskSizeWidth,
		nMaskSizeHeight);
	dyn_threshold(imgReduce,imgMean,&regThresh,pSGenReg.nEdge,"dark");
	if (pSGenReg.nGray>0)
	{
		threshold(imgReduce,&regBinThresh,10,pSGenReg.nGray);//0---10  20180821 防止全0 或者无瓶图像 
		union2(regBinThresh,regThresh,&regThresh);
	}
	//2015.1.4增加平滑灰度参数：方圆光源反光导致图像有亮条纹，设定该参数,用于排除亮条纹区域干扰
	if (pSGenReg.nMeanGray<255)
	{
		threshold(imgMean,&regTemp,pSGenReg.nMeanGray,255);
		difference(regThresh,regTemp,&regThresh);
	}
	////开运算
	//if (pSGenReg.bOpening)
	//{
	//	opening_circle(regThresh,&regThresh,pSGenReg.fOpeningSize);
	//}
	//排除干扰-外部干扰区域
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal;
			if (pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg,&xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect,&xldVal, 1, -1);
			}

			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(oSGenReg.oValidRegion,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regThresh,regDisturb,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
			case 1://典型缺陷				
				intersection(regThresh,regDisturb,&regTemp);
				difference(regThresh,regDisturb,&regThresh);
				opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
				concat_obj(regThresh,regTemp,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				break;
			case 2://排除条纹
				{
					Hobject regStripe;
					//2013.9.16 nanjc 增独立参数提取排除
					if (pDistReg.bStripeSelf)
					{
						distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
					}
					else
					{
						intersection(regDisturb,regThresh,&regTemp);
						distRegStripe(pDistReg,regTemp,&regStripe);					
					}
					difference(regThresh,regStripe,&regThresh);
					concat_obj(regStripe,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				}
				break;
			case 3://排除气泡
				{
					// 2018.2-瓶身无需排除气泡
					//Hobject regBubble;
					//intersection(regDisturb,regThresh,&regTemp);
					//distBubble(imgSrc,pDistReg,regTemp,&regBubble);	//提取要排除的气泡
					//difference(regThresh,regBubble,&regThresh);
					//concat_obj(regBubble,objSpecial,&objSpecial);
					//ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_BUBBLE,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				}
				break;
			default:
				break;
			}
		}
	}

	//排除干扰-内部干扰设置
	if (pSGenReg.bDistCon1 || pSGenReg.bDistCon2)
	{
		Hobject regStripe,regSel;
		double Phi;
		gen_empty_obj(&regTemp);
		if (pSGenReg.bDistEdge && pSGenReg.nDistEdge>0)
		{
			dyn_threshold(imgReduce,imgMean,&regBlob,pSGenReg.nDistEdge,"dark");
			difference(regBlob,objSpecial,&regBlob);//去掉完全不检
		}
		else
		{
			union1(regThresh, &regBlob);
		}
	
		connection(regBlob, &regDisturb);
		select_shape(regDisturb, &regDisturb, "area", "and", 10, 9999999); //条纹至少得>10 pix

		if (pSGenReg.bDistCon1) //排除垂直条纹
		{
			gen_empty_obj(&regStripe);
			closing_circle(regDisturb,&regDisturb,3);  //2018.3-区分竖直薄皮气泡与条纹
			select_shape(regDisturb, &regDisturb, "inner_radius", "or", 0.5*pSGenReg.nDistInRadiusL1,0.5*pSGenReg.nDistInRadiusH1);
			count_obj(regDisturb,&nNum);
			for (int i=0;i<nNum;i++)
			{
				select_obj(regDisturb,&regSel,i+1);
				elliptic_axis(regSel, NULL, NULL, &Phi);
				tuple_deg(Phi,&Phi);
				Phi=Phi<0?Phi+180.f:Phi;
				if ( fabs(90.f-Phi)<pSGenReg.nDistVerPhi)
				{
					concat_obj(regStripe,regSel,&regStripe);
				}
			}
			if (pSGenReg.nDistInRadiusH1>=pSGenReg.nDistInRadiusL1 && pSGenReg.nDistAniH1>=pSGenReg.nDistAniL1)
			{
				select_shape(regStripe,&regStripe,"inner_radius","and",0.5*pSGenReg.nDistInRadiusL1,0.5*pSGenReg.nDistInRadiusH1);
				select_shape(regStripe,&regStripe,HTuple("anisometry").Concat("anisometry"),"or",HTuple(pSGenReg.nDistAniL1).Concat(0),HTuple(pSGenReg.nDistAniH1).Concat(0));//长宽比
				concat_obj(regTemp,regStripe,&regTemp);
			}
/*			difference(regBlob, regStripe, &regDiff);
			ExtExcludeDefect(rtnInfo,regBlob,regDiff,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_DISTCON1,pSGenReg.strName);	*/	
		}
		if (pSGenReg.bDistCon2) //排除水平条纹
		{
            gen_empty_obj(&regStripe);
			count_obj(regDisturb,&nNum);
			for (int i=0;i<nNum;i++)
			{
				select_obj(regDisturb,&regSel,i+1);
				elliptic_axis(regSel, NULL, NULL, &Phi);
				tuple_deg(Phi,&Phi);
				if ( fabs(Phi)<pSGenReg.nDistHorPhi )
				{
					concat_obj(regStripe,regSel,&regStripe);
				}
			}
			if (pSGenReg.nDistInRadiusH2>=pSGenReg.nDistInRadiusL2 && pSGenReg.nDistAniH2>=pSGenReg.nDistAniL2)
			{
				select_shape(regStripe,&regStripe,"inner_radius","and",0.5*pSGenReg.nDistInRadiusL2,0.5*pSGenReg.nDistInRadiusH2);
				select_shape(regStripe,&regStripe,HTuple("anisometry").Concat("anisometry"),"or",HTuple(pSGenReg.nDistAniL2).Concat(0),HTuple(pSGenReg.nDistAniH2).Concat(0));//长宽比
				concat_obj(regTemp, regStripe, &regTemp);
			}
			/*			difference(regBlob, regDisturb, &regDiff);
			ExtExcludeDefect(rtnInfo,regBlob,regDiff,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_DISTCON2,pSGenReg.strName);	*/	
		}
		concat_obj(objSpecial, regTemp, &objSpecial); //2017.5---将条纹添加到排除干扰中
		difference(regThresh, regTemp, &regThresh);
	}
	
	//开运算---2017.6：把开运算位置移到排除条纹后
	//原因：身普区同时有条纹和花纹时，先开运算，导致条纹提取不全，不能完全排除
	if (pSGenReg.bOpening)
	{
		opening_circle(regThresh,&regThresh,pSGenReg.fOpeningSize);
	}

	//分析缺陷类型
	union1(regThresh,&regThresh);
	connection(regThresh,&regThresh);
	select_shape(regThresh,&regBlob,HTuple("area").Concat("ra"),"or",
		HTuple(max(1,pSGenReg.nArea)).Concat(max(1,pSGenReg.nLength/2)),  //2017.2---改进（面积或长度为0时，误报气泡：0）
		HTuple(99999999).Concat(99999));
	ExtExcludeDefect(rtnInfo,regThresh,regBlob,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pSGenReg.strName);
	count_obj(regBlob,&nNum);
	if (nNum>0)//报面积最大的一个
	{
		select_shape_std(regBlob,&regTemp,"max_area",70);
		elliptic_axis(regTemp,&Ra,&Rb,NULL);
		Rb = (Rb==0)?0.5:Rb;
		compactness(regTemp,&Comp);
		convexity(regTemp,&Conv);
		if (Ra/Rb > 4 || Comp >5)
		{
			rtnInfo.nType = ERROR_CRACK;
		}
		else if (Conv < 0.75)
		{
			rtnInfo.nType = ERROR_BUBBLE;
		}
		else 
		{
			rtnInfo.nType = ERROR_SPOT;
		}
		concat_obj(rtnInfo.regError,regBlob,&rtnInfo.regError);
		return rtnInfo;
	}	

	//小结石检测
	gen_empty_obj(&regBlob);
	if (pSGenReg.bStone)
	{
		dyn_threshold(imgReduce,imgMean,&regBlob,pSGenReg.nStoneEdge,"dark");
		difference(regBlob,objSpecial,&regBlob);//去掉完全不检、典型缺陷和条纹区域

		connection(regBlob,&regBlob);
		select_shape(regBlob,&regTemp,"area","and",1,99999999);
		count_obj(regTemp,&nNum);
		if (nNum!=0)
		{
			select_shape_proto(regTemp,oSGenReg.oValidRegion,&regBlob,"distance_contour",2,9999);//去掉靠边区域,传递空region有异常
			ExtExcludeDefect(rtnInfo,regTemp,regBlob,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_STONE,pSGenReg.strName);
			select_shape(regBlob,&regTemp,"area","and",pSGenReg.nStoneArea,999999);
			ExtExcludeDefect(rtnInfo,regBlob,regTemp,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_STONE,pSGenReg.strName);
			select_shape(regTemp,&regBlob,"circularity","and",0.3,1.0);
			ExtExcludeDefect(rtnInfo,regTemp,regBlob,EXCLUDE_CAUSE_CIRCULARITY,EXCLUDE_TYPE_STONE,pSGenReg.strName,false);
			count_obj(regBlob,&nNum);
			if (nNum>=pSGenReg.nStoneNum)
			{
				concat_obj(regBlob,rtnInfo.regError,&rtnInfo.regError);
				rtnInfo.nType = ERROR_STONE;
				return rtnInfo;
			}
		}		
	}

	//小黑点检测
	gen_empty_obj(&regBlob);
	if (pSGenReg.bDarkdot)
	{
		dyn_threshold(imgReduce,imgMean,&regBlob,pSGenReg.nDarkdotEdge,"dark");
		difference(regBlob,objSpecial,&regBlob);//去掉完全不检、典型缺陷和条纹区域

		connection(regBlob,&regBlob);
		select_shape(regBlob,&regTemp,"area","and",1,99999999);
		count_obj(regTemp,&nNum);
		if (nNum!=0)
		{
			select_shape_proto(regTemp,oSGenReg.oValidRegion,&regBlob,"distance_contour",2,9999);//去掉靠边区域
			ExtExcludeDefect(rtnInfo,regTemp,regBlob,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_DARKDOT,pSGenReg.strName);
			select_shape(regBlob,&regTemp,"area","and",pSGenReg.nDarkdotArea,999999);
			ExtExcludeDefect(rtnInfo,regBlob,regTemp,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_DARKDOT,pSGenReg.strName);
			select_shape(regTemp,&regBlob,"circularity","and",pSGenReg.fDarkdotCir,1.0); //0.3
			ExtExcludeDefect(rtnInfo,regTemp,regBlob,EXCLUDE_CAUSE_CIRCULARITY,EXCLUDE_TYPE_DARKDOT,pSGenReg.strName);
			select_shape(regBlob,&regTemp,"anisometry","and",1,2.5); //2017.11-添加长宽比限制
			ExtExcludeDefect(rtnInfo,regBlob,regTemp,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_DARKDOT,pSGenReg.strName);
			count_obj(regTemp,&nNum);
			if (nNum>=pSGenReg.nDarkdotNum)
			{
				concat_obj(regTemp,rtnInfo.regError,&rtnInfo.regError);
				rtnInfo.nType = ERROR_DARKDOT;
				return rtnInfo;
			}
		}		
	}

	// 微裂纹检测
	gen_empty_obj(&regLigStripe);
	if (pSGenReg.bTinyCrack)
	{
		dyn_threshold(imgReduce,imgMean,&regThresh,pSGenReg.nTinyCrackEdge,"dark");
		difference(regThresh,objSpecial,&regThresh);//去掉完全不检、典型缺陷和条纹区域
		closing_circle(regThresh,&regThresh,1.5);
		connection(regThresh,&regThresh);
		select_shape(regThresh,&regThresh,"area","and",5,99999999);	//面积
		//select_shape_proto(regTemp, oSGenReg.oValidRegion, &regThresh, "distance_contour",2, 10000);//2014.5.22 排除靠边
		//ExtExcludeDefect(rtnInfo,regTemp,regThresh,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_TINYCRACK,pSGenReg.strName);
		count_obj(regThresh,&nNum);
		if (nNum>0)
		{
			select_shape(regThresh,&regLigStripe,"ra","and",pSGenReg.nTinyCrackLength/2,999999);		
			ExtExcludeDefect(rtnInfo,regThresh,regLigStripe,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_TINYCRACK,pSGenReg.strName);			
			select_shape(regLigStripe,&regTemp,HTuple("anisometry").Concat("anisometry"),"or",HTuple(pSGenReg.nTinyCrackAnsi).Concat(0),HTuple(999999).Concat(0));//长宽比
			ExtExcludeDefect(rtnInfo,regLigStripe,regTemp,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_TINYCRACK,pSGenReg.strName);			
			select_shape(regTemp,&regLigStripe,"inner_radius","and",0,0.5*pSGenReg.nTinyCrackInRadius);
			ExtExcludeDefect(rtnInfo,regTemp,regLigStripe,EXCLUDE_CAUSE_INRADIUS,EXCLUDE_TYPE_TINYCRACK,pSGenReg.strName);			
			if (pSGenReg.nTinyCrackPhiH >= pSGenReg.nTinyCrackPhiL)
			{
				select_shape(regLigStripe, &regTemp, HTuple("phi").Concat("phi"), "or", HTuple(-0.0175*pSGenReg.nTinyCrackPhiH).Concat(0.0175*pSGenReg.nTinyCrackPhiL), HTuple(-0.0175*pSGenReg.nTinyCrackPhiL).Concat(0.0175*pSGenReg.nTinyCrackPhiH));
				ExtExcludeDefect(rtnInfo,regLigStripe,regTemp,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_TINYCRACK,pSGenReg.strName);
			}
			count_obj(regTemp,&nNum);
			if (nNum>0)
			{
				concat_obj(regTemp,rtnInfo.regError,&rtnInfo.regError);
				rtnInfo.nType = ERROR_TINYCRACK;	//报裂纹
				return rtnInfo;
			}
		}
	}

	//亮条纹检测
	gen_empty_obj(&regLigStripe);
	if (pSGenReg.bLightStripe)
	{
		dyn_threshold(imgReduce,imgMean,&regThresh,pSGenReg.nLightStripeEdge,"light");
		difference(regThresh,objSpecial,&regThresh);//去掉完全不检、典型缺陷和条纹区域
		closing_circle(regThresh,&regThresh,1.5);

		connection(regThresh,&regThresh);
		select_shape(regThresh,&regThresh,"area","and",10,99999999);	//面积
		select_shape(regThresh,&regTemp,"anisometry","and",5,999999);//长宽比
		select_shape_proto(regTemp, oSGenReg.oValidRegion, &regThresh, "distance_contour",2, 10000);//2014.5.22 排除靠边
		ExtExcludeDefect(rtnInfo,regTemp,regThresh,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_LIGHTSTRIPE,pSGenReg.strName);
		count_obj(regThresh,&nNum);
		if (nNum>0)
		{
			select_shape(regThresh,&regLigStripe,"ra","and",pSGenReg.nLightStripeLength/2,999999);
			ExtExcludeDefect(rtnInfo,regThresh,regLigStripe,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_LIGHTSTRIPE,pSGenReg.strName);
			select_shape(regLigStripe,&regTemp,"inner_radius","and",0,0.5*pSGenReg.nLightStripeInRadius);
			ExtExcludeDefect(rtnInfo,regLigStripe,regTemp,EXCLUDE_CAUSE_INRADIUS,EXCLUDE_TYPE_LIGHTSTRIPE,pSGenReg.strName);
			if (pSGenReg.nLightStripePhiH >= pSGenReg.nLightStripePhiL)
			{
				select_shape(regTemp, &regLigStripe, HTuple("phi").Concat("phi"), "or", HTuple(-0.0175*pSGenReg.nLightStripePhiH).Concat(0.0175*pSGenReg.nLightStripePhiL), HTuple(-0.0175*pSGenReg.nLightStripePhiL).Concat(0.0175*pSGenReg.nLightStripePhiH));
				ExtExcludeDefect(rtnInfo,regTemp,regLigStripe,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_LIGHTSTRIPE,pSGenReg.strName);
			}
			count_obj(regLigStripe,&nNum);
			if (nNum>0)
			{
				concat_obj(regLigStripe,rtnInfo.regError,&rtnInfo.regError);
				rtnInfo.nType = ERROR_CRACK;	//亮条纹-暂报裂纹
				return rtnInfo;
			}
		}		
	}

	////2017.5---薄皮气泡检测
	//gen_empty_obj(&regBubbles);
	//bool isBubble;
	//if (pSGenReg.bBubbles && pSGenReg.nBubblesHighThres>=pSGenReg.nBubblesLowThres)
	//{		 
	//	long hei,wid;
	//	//inner_circle (oSGenReg.oValidRegion, NULL, NULL,&radius); //区域大时很耗时
	//	smallest_rectangle1 (oSGenReg.oValidRegion, &Row1, &Col1, &Row2, &Col2);
	//	hei = Row2 - Row1+1;
	//	wid = Col2 - Col1+1;
	//	if (hei>10 && wid>10) //2017.7-edges_image滤波模板至少为3x3
	//	{	
	//		// 提取检测区域内所有疑似气泡	
	//		edges_image (imgReduce, &ImaAmp, &ImaDir, "sobel_fast", 1, "nms", pSGenReg.nBubblesLowThres, pSGenReg.nBubblesHighThres); //canny->sobel_fast，减少检测时间
	//		threshold (ImaAmp, &RegDark, 0, 255); 
	//		difference(RegDark,objSpecial,&RegDark);//去掉完全不检、典型缺陷和条纹区域
	//		closing_circle (RegDark, &RegDarkClo, 3.5);
	//		connection (RegDarkClo, &RegCon);
	//		select_shape (RegCon, &RegCon, "area", "and", 20, 99999999);
	//		select_shape_proto(RegCon, oSGenReg.oValidRegion, &BubbleCan, "distance_contour",2, 10000);//排除靠边
	//		ExtExcludeDefect(rtnInfo,RegCon,BubbleCan,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_THIN_BUBBLE,pSGenReg.strName);
	//		select_shape (BubbleCan, &RegCon, "anisometry", "and", 1, 5);
	//		ExtExcludeDefect(rtnInfo,BubbleCan,RegCon,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_THIN_BUBBLE,pSGenReg.strName);
	//		select_shape (RegCon, &BubbleCan, "area", "and", pSGenReg.nBubblesArea, 9999999);
	//		ExtExcludeDefect(rtnInfo,RegCon,BubbleCan,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_THIN_BUBBLE,pSGenReg.strName); 
	//		count_obj (BubbleCan, &nNum);  

	//		if (nNum>0)
	//		{
	//			// 逐个分析是否为气泡
	//			for (int i=0; i<nNum; i++)
	//			{
	//				isBubble = FALSE;
	//				select_obj (BubbleCan, &SelObj, i+1);

	//				// 先用灰度判断：气泡中心灰度值大于边缘灰度值
	//				intensity (SelObj, imgSrc, &Mean1, NULL);
	//				area_center (SelObj, NULL, &RowCen, &ColCen);
	//				zoom_region (SelObj, &RegionZoom, 0.5, 0.5);
	//				area_center (RegionZoom, NULL, &RowCenZoom, &ColCenZoom);
	//				move_region (RegionZoom, &RegionMoved, RowCen-RowCenZoom, ColCen-ColCenZoom);
	//				intensity (RegionMoved, imgSrc, &Mean2, NULL);
	//				if (Mean2-Mean1 < pSGenReg.nBubblesGrayOffset) //灰度差
	//				{
	//					difference(BubbleCan, SelObj, &regTemp);
	//					ExtExcludeDefect(rtnInfo,BubbleCan,regTemp,EXCLUDE_CAUSE_GRAY_DIFFERENCE,EXCLUDE_TYPE_THIN_BUBBLE,pSGenReg.strName);
	//					continue;
	//				}

	//				// 再用形状特征来判断:正常气泡有内外两层轮廓
	//				shape_trans (SelObj, &RegTrans, "outer_circle");
	//				dilation_circle (RegTrans, &RegDil, 2);
	//				intersection (RegDil, RegDark, &RegInter);
	//				connection (RegInter, &RegCon);
	//				select_shape (RegCon, &RegSel, "area", "and", 5, 99999999);
	//				count_obj (RegSel, &count);           
	//				if (count == 1)
	//				{
	//					copy_obj (RegSel, &OutReg, 1, -1);
	//				}
	//				else if (count > 1)
	//				{
	//					area_center (RegSel, &area, NULL, NULL);
	//					tuple_max (area, &areaMax);
	//					tuple_find (area, areaMax, &ind);
	//					select_obj (RegSel, &OutReg, ind[0].L()+1);   //找到气泡外环

	//					// 气泡外环与内部轮廓断开
	//					difference (RegInter, OutReg, &RegOther);
	//					connection (RegOther, &RegOtherCon);
	//					count_obj (RegOtherCon, &count);
	//					if (count > 0)
	//					{
	//						for (int j=1; j<=count; j++)
	//						{
	//							select_obj (RegOtherCon, &RegSel, j);
	//							area_center (RegSel, NULL, &CenRow, &CenCol);
	//							shape_trans (OutReg, &OutRegCir, "convex");
	//							test_region_point (OutRegCir, CenRow, CenCol, &IsInside);
	//							if (IsInside)
	//							{
	//								isBubble = TRUE;
	//								concat_obj (regBubbles, SelObj, &regBubbles); //OutReg->SelObj
	//								break;
	//							}
	//						}
	//					}
	//				}

	//				if (!isBubble)
	//				{
	//					// 气泡外环与内部轮廓相连
	//					fill_up (OutReg, &OutRegFillup);
	//					area_center (OutRegFillup, &areaFillup, NULL, NULL);
	//					opening_circle (OutRegFillup, &RegOpening, 1);
	//					area_center (RegOpening, &areaOpening, NULL, NULL);
	//					if (areaFillup-areaOpening > 10)
	//					{
	//						//有误检，待添加
	//					}
	//					else
	//					{
	//						// 气泡外环闭合
	//						difference (OutRegFillup, OutReg, &RegDiff);
	//						area_holes (RegDiff, &area);               
	//						if (area > 2)
	//						{
	//							concat_obj (regBubbles, SelObj, &regBubbles);  //OutReg->SelObj
	//						}
	//					}
	//				}				
	//			} //endfor
	//		}
	//		count_obj (regBubbles, &nNum);
	//		if (nNum>0)
	//		{
	//			concat_obj(regBubbles,rtnInfo.regError,&rtnInfo.regError);
	//			rtnInfo.nType = ERROR_THIN_BUBBLE;	
	//			return rtnInfo;
	//		}
	//	}
 //   }//endif(bBubbles)

	//2018.1---薄皮气泡检测算法修改
	if (pSGenReg.bBubbles && pSGenReg.nBubblesHighThres>=pSGenReg.nBubblesLowThres)
	{
		Hobject eroReg,ImageEmphasize,Edges,EdgesSel,DefectsTemp;
		Hobject ObjTemp,Region,regInter,regSel,ObjSel,RegionZoom,RegionMoved,RegionZoomIn;
		HTuple Row,Col,PhiDiff,DegDiff;
		HTuple RowCen,ColCen,RowCenZoom,ColCenZoom,RowCenZoomIn,ColCenZoomIn;
		Hlong num/*,count*/;
		double Anisometry,Phi,Mean1,Mean2,Mean3;	
		//erosion_circle (oSGenReg.oValidRegion, &eroReg, 3);

		// 查找缺陷
		//emphasize (imgReduce, &ImageEmphasize, 11, 11, 1);
		edges_sub_pix (imgReduce, &Edges, "lanser2", 0.4, pSGenReg.nBubblesLowThres, pSGenReg.nBubblesHighThres); //10,20-lanser2时间较长
		//union_adjacent_contours_xld (Edges, UnionEdges, 5, 1, 'attr_keep')
		select_shape_xld (Edges, &EdgesSel, "contlength", "and", 20, 999999);
		select_shape_xld (EdgesSel, &EdgesSel,"circularity", "and", pSGenReg.fBubblesCir, 1); //不圆的薄皮气泡待改进
		//ExtExcludeDefect(rtnInfo,DefectsTemp,ObjTemp,EXCLUDE_CAUSE_CIRCULARITY,EXCLUDE_TYPE_THIN_BUBBLE,pSGenReg.strName); //待修改xld->region
		select_shape_xld (EdgesSel, &EdgesSel,"contlength", "and", pSGenReg.nBubblesLength, 999999);
		//ExtExcludeDefect(rtnInfo,ObjTemp,ObjSel,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_THIN_BUBBLE,pSGenReg.strName);

		// 缺陷筛选-排除竖直方向的线条（例如模缝线）
		count_obj (EdgesSel, &num);
		if (num > 0)
		{
			for (int ind=0;ind<num;ind++)
			{
				select_obj (EdgesSel, &ObjTemp, ind+1);
				eccentricity_xld (ObjTemp, &Anisometry, NULL, NULL);
				smallest_rectangle2_xld (ObjTemp, NULL, NULL, &Phi, NULL, NULL);
				PhiDiff = fabs(fabs(Phi)-1.57);
				DegDiff = PhiDiff.Deg();
				if ( (Anisometry==0 || Anisometry>5) && DegDiff<10) //认为是模缝线或其他干扰
				{	/*do nothing*/ }
				else
				{
					get_contour_xld (ObjTemp, &Row, &Col);
					gen_region_points (&Region, Row, Col);//XLD->region

					//灰度筛选：气泡中心比边缘黑太多，肯定不是气泡
					//          气泡中心比气泡边缘外背景亮太多，也不是气泡
					intensity (Region, imgReduce, &Mean1, NULL);
					area_center (Region, NULL, &RowCen, &ColCen);
					zoom_region (Region, &RegionZoom, 0.5, 0.5);
					area_center (RegionZoom, NULL, &RowCenZoom, &ColCenZoom);
					move_region (RegionZoom, &RegionMoved, RowCen-RowCenZoom, ColCen-ColCenZoom);
					intensity (RegionMoved, imgReduce, &Mean2, NULL);

					zoom_region (Region, &RegionZoomIn, 1.5, 1.5);
					area_center (RegionZoomIn, NULL, &RowCenZoomIn, &ColCenZoomIn);
					move_region (RegionZoomIn, &RegionMoved, RowCen-RowCenZoomIn, ColCen-ColCenZoomIn);
					intensity (RegionMoved, imgReduce, &Mean3, NULL);
					if ( (Mean1-Mean2 > 20) || (Mean2-Mean3 > 20)) //灰度差
					{
						continue;
					}
					else
					{
						concat_obj (rtnInfo.regError, Region, &rtnInfo.regError);
					}	
				}
			}
			count_obj(rtnInfo.regError, &num);
			if (num > 0)
			{
				rtnInfo.nType = ERROR_THIN_BUBBLE;	
				return rtnInfo;
			}  
		}
	}//endif(bBubbles)

	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：检测区域预处理函数
RtnInfo CCheck::genValidROI(Hobject &imgSrc,s_ROIPara &roiPara,Hobject &ROI,Hobject *validROI)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = ERROR_INVALID_ROI;
	rtnInfo.strEx=QObject::tr("Pretreatment region mistake");

	Hlong nRow1,nCol1,nRow2,nCol2;
	Hobject imgReduced;
	Hobject PartionRegion,RegionBin,ConnectedRegions,SelectedRegions;

	int nPartNum;//分割份数	
	s_pSideLoc pSideLoc;
	if (m_bLocate && vModelParas[m_nLocateItemID].canConvert<s_pSideLoc>())
	{		
		pSideLoc = vModelParas[m_nLocateItemID].value<s_pSideLoc>();
	}
	reduce_domain(imgSrc,ROI,&imgReduced);
	smallest_rectangle1(ROI,&nRow1,&nCol1,&nRow2,&nCol2);
	int nRegionHeight = nRow2-nRow1+1;
	float fHeiRatio = (float)nRegionHeight/(float)m_nHeight;
	if (fHeiRatio<0.3)
	{
		nPartNum=1;
	}
	else if (fHeiRatio>=0.3 && fHeiRatio<0.6)
	{
		nPartNum=2;
	}
	else
	{
		nPartNum=3;
	}
	int nPartHeight = nRegionHeight/nPartNum + 2;	
	partition_rectangle(ROI, &PartionRegion, m_nWidth, nPartHeight);
	HTuple Mean, PreMean;
	intensity(PartionRegion, imgSrc, &Mean, NULL);
	PreMean = Mean * roiPara.fRoiRatio;

	Hobject selected, tempRegion;
	gen_empty_obj(&RegionBin);

	// 在区域较小时分割后的区域个数会小于分割参数
	Hlong nNumber;
	count_obj(PartionRegion, &nNumber);

	for (int i = 0; i< nNumber; ++i)
	{
		select_obj(PartionRegion, &selected, i+1);
		reduce_domain(imgSrc, selected, &tempRegion); 
		threshold(tempRegion,&tempRegion,HTuple(PreMean[i]),HTuple(255));
		concat_obj(RegionBin, tempRegion, &RegionBin);			
	}
	union1(RegionBin, &RegionBin);

	Hlong areaPro, areaMin;
	area_center(ROI, &areaPro, NULL, NULL);
	areaMin = areaPro/3;
	areaMin = areaMin > 550 ? 550:areaMin;  

	// 填充裂纹区域
	closing_rectangle1(RegionBin, &RegionBin, 1, roiPara.nClosingWH);
	closing_rectangle1(RegionBin, &RegionBin, roiPara.nClosingWH, 1);

	connection(RegionBin, &ConnectedRegions);
	select_shape(ConnectedRegions, &SelectedRegions, HTuple("area"),
		HTuple("and"), HTuple(areaMin), HTuple(99999999));

	// 判断是几个区域
	count_obj(SelectedRegions, &nNumber);

	// 排除左右两边的预处理区域
	if (nNumber > 1 && m_bLocate)
	{
		if (pSideLoc.nMethodIdx == 1)//左右平移定位时
		{
			select_shape_std(SelectedRegions,&SelectedRegions,"max_area",70);
		}
		else if(pSideLoc.nMethodIdx ==0)//平移旋转时
		{
			SideOrigin sOri = vModelShapes[m_nLocateItemID].value<s_oSideLoc>().ori;

			double ColLeft, ColRight;
			ColLeft = (sOri.nCol11+sOri.Col)/2.0;
			ColRight = (sOri.nCol12+sOri.Col)/2.0;
			ColRight = ColRight > ColLeft+10 ? ColRight : ColLeft+10;

			select_shape(SelectedRegions, &SelectedRegions, "column", "and", ColLeft, ColRight);
			count_obj(SelectedRegions, &nNumber);

			if(nNumber > 1)
			{
				// 排除由于模缝线将一个区域分成两份的情况
				Hlong nCloWid = 10;//模缝线宽
				nCloWid = nCloWid > 0 ? nCloWid : 5;
				union1(SelectedRegions, &SelectedRegions);
				closing_rectangle1(SelectedRegions, &SelectedRegions, nCloWid, 1);

				connection(SelectedRegions, &ConnectedRegions);
				select_shape(ConnectedRegions, &SelectedRegions, HTuple("area"),
					HTuple("and"), HTuple(areaMin), HTuple(99999999));

				count_obj(SelectedRegions, &nNumber);
				if(nNumber > 1)
				{
					concat_obj(rtnInfo.regError,SelectedRegions,&rtnInfo.regError);					
					return rtnInfo;
				}
				else if (nNumber == 0)
				{
					concat_obj(rtnInfo.regError,ROI,&rtnInfo.regError);					
					return rtnInfo;
				}
			}
			else if (nNumber == 0)
			{
				concat_obj(rtnInfo.regError,ROI,&rtnInfo.regError);				
				return rtnInfo;
			}

		}

	}
	else if (nNumber == 0)
	{
		concat_obj(rtnInfo.regError,ROI,&rtnInfo.regError);		
		return rtnInfo;
	}

	// 找出预处理区域后缩小一定范围进行检测
	// 主要作用是切断那些裂纹,大缺陷
	fill_up(SelectedRegions, &SelectedRegions);

	// 判断实际处理区域比设置的处理区域小很多的情况
	Hobject RegGap;
	difference(ROI, SelectedRegions, &RegGap);
	opening_rectangle1(RegGap, &RegGap, roiPara.nGapWH, roiPara.nGapWH);
	connection(RegGap, &RegGap);
	select_shape(RegGap, &RegGap, HTuple("width").Concat("height"), "and", 
		HTuple(roiPara.nGapWH).Concat(roiPara.nGapWH),
		HTuple(9999).Concat(9999));
	count_obj(RegGap, &nNumber);
	if (nNumber > 0)
	{
		concat_obj(rtnInfo.regError, RegGap, &rtnInfo.regError);
		rtnInfo.strEx=QObject::tr("The actual detection region exist breach.");
		return rtnInfo;
	}

	// 往内部缩小一定区域
	erosion_circle(SelectedRegions, &SelectedRegions, HTuple(2.5));
	//2013.9.16 nanjc 一个区域缩为两个区域，选大区域
	connection(SelectedRegions, &SelectedRegions);
	count_obj(SelectedRegions, &nNumber);
	if (nNumber>1)
	{
		select_shape_std(SelectedRegions,&SelectedRegions,"max_area",70);
	}

	copy_obj(SelectedRegions,validROI,1,-1);
	rtnInfo.nType = GOOD_BOTTLE;
	rtnInfo.strEx.clear();
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：干扰区域,返回满足条件的干扰区域
void CCheck::distRegStripe(Hobject &imgSrc,s_pDistReg &pDistReg,s_oDistReg &oDistReg,Hobject *regStripe)
{
	Hobject regCheck,regThresh,regSel;
	Hobject imgReduce,imgMean;	
	Hlong nNum;
	int nMaskSize = pDistReg.nStripeMaskSize;//算子尺度
	int nEdge = pDistReg.nStripeEdge<5?5:pDistReg.nStripeEdge;//对比度
	gen_empty_obj(regStripe);
	if (pDistReg.nShapeType == 0)
	{
		gen_region_contour_xld(oDistReg.oDisturbReg,&regCheck,"filled");
	}
	else
	{
		gen_region_contour_xld(oDistReg.oDisturbReg_Rect,&regCheck,"filled");
	}
	
	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regCheck,&regCheck,"area","and",100,99999999);
	count_obj(regCheck,&nNum);
	if (nNum==0)
	{
		return;
	}

	reduce_domain(imgSrc,regCheck,&imgReduce);
	mean_image(imgReduce,&imgMean,nMaskSize,nMaskSize);
	dyn_threshold(imgReduce,imgMean,&regThresh,nEdge,"dark");

	distRegStripe(pDistReg,regThresh,regStripe);
}

void CCheck::distRegStripe(s_pDistReg &pDistReg,Hobject &ROI,Hobject *regStripe)
{
	Hobject regThresh,regSel;
	Hlong nNum;
	int i;
	double Ra,Rb,Phi,InRadius;
	gen_empty_obj(regStripe);
	connection(ROI,&regThresh);
	count_obj(regThresh,&nNum);

	for (i=0;i<nNum;++i)
	{
		select_obj(regThresh,&regSel,i+1);
		elliptic_axis(regSel, &Ra, &Rb, &Phi);
		Rb = (Rb==0)?0.5:Rb;
		tuple_deg(Phi,&Phi);
		inner_circle(regSel,NULL,NULL,&InRadius);
		if (pDistReg.bHoriStripe)//水平条纹
		{
			if (fabs(Phi)<pDistReg.nHoriAng && 
				Rb*2>pDistReg.nHoriWidthL && Rb*2<pDistReg.nHoriWidthH &&
				Ra/Rb>pDistReg.fHoriRabL && Ra/Rb<pDistReg.fHoriRabH&&
				InRadius*2>pDistReg.fHoriInRadiusL&&InRadius*2<pDistReg.fHoriInRadiusH)
			{
				concat_obj(*regStripe,regSel,regStripe);
			}
		}
		if (pDistReg.bVertStripe)//垂直条纹
		{
			Phi=Phi<0?Phi+180.f:Phi;
			if ((0 == Rb) || fabs(90.f-Phi)<pDistReg.nVertAng &&
				Rb*2>pDistReg.nVertWidthL && Rb*2<pDistReg.nVertWidthH &&
				Ra/Rb>pDistReg.fVertRabL && Ra/Rb<pDistReg.fVertRabH&&
				InRadius*2>pDistReg.fVertInRadiusL&&InRadius*2<pDistReg.fVertInRadiusH)
			{
				concat_obj(*regStripe,regSel,regStripe);
			}
		}
	}
	union1(*regStripe,regStripe);
}

// 2017.4---排除气泡干扰（ROI为初步提取的缺陷）
void CCheck::distBubble(Hobject &imgSrc,s_pDistReg &pDistReg,Hobject &ROI,Hobject *regBubble)
{
	Hobject regThresh,regSel,regSelDil,ImgReduced,ImaAmp,ImaDir;
	Hobject RegDark,RegCon,OutReg;
	Hobject RegOther,RegOtherCon,ObjSel,OutRegCir;
	Hobject OutRegFillup,RegDiff;
	HTuple RegDiffArea,areaHoles,areaMax,ind,CenRow,CenCol;	
	Hlong nNum,num,count,IsInside;
	int i;
	bool IsBubble;

	IsBubble = false;
	gen_empty_obj(regBubble);
	connection(ROI, &regThresh);
	select_shape(regThresh, &regThresh, "circularity","and",pDistReg.fBubbleCir,1);
	select_shape(regThresh, &regThresh, "area","and",1,pDistReg.nBubbleArea); //改为上限，排除小气泡
	count_obj(regThresh,&nNum);

	// 单个分析是否为气泡
	for (i=0;i<nNum;++i)
	{
		select_obj(regThresh,&regSel,i+1);
		dilation_circle(regSel,&regSelDil,3);
		reduce_domain (imgSrc, regSelDil, &ImgReduced);
		edges_image (ImgReduced, &ImaAmp, &ImaDir, "canny", 1, "nms", pDistReg.nBubbleLowThre, pDistReg.nBubbleHighThre);
		threshold (ImaAmp, &RegDark, 0, 255);
		connection (RegDark, &RegCon);
		count_obj (RegCon, &num);
		if (num > 0)
		{
			//找到气泡外环OutReg
			if (num == 1)
			{
				copy_obj (RegCon, &OutReg, 1, -1);
			}
			else
			{
				area_holes (RegCon, &areaHoles);
				tuple_max (areaHoles, &areaMax);
				tuple_find (areaHoles, areaMax, &ind);
				select_obj (RegCon, &OutReg, ind+1);   //气泡外轮廓

				// 判断气泡外轮廓是否与内部轮廓断开
				difference (RegDark, OutReg, &RegOther);
				connection (RegOther, &RegOtherCon);
				count_obj (RegOtherCon, &count);
				if (count > 0)
				{
					for( int j=1;j<=count;j++)
					{
						select_obj (RegOtherCon, &ObjSel, j);
						area_center (ObjSel, NULL, &CenRow, &CenCol);
						shape_trans (OutReg, &OutRegCir, "outer_circle");
						test_region_point (OutRegCir, CenRow, CenCol, &IsInside);
						if (IsInside)
						{
                             IsBubble = true;
							 concat_obj(*regBubble,regSel,regBubble);
			                 break;
						}
					}
				}
			} // endif (num == 1)

			if (!IsBubble)
			{ 
				// 判断气泡外轮廓是否与内部轮廓相连
				fill_up (OutReg, &OutRegFillup);
				difference (OutRegFillup, OutReg, &RegDiff);
				area_holes (RegDiff, &RegDiffArea);
				if (RegDiffArea > 2) 
				{
					IsBubble = true;
					concat_obj(*regBubble,regSel,regBubble);
				}
			}		 
		}
	}
	union1(*regBubble,regBubble);
}


//*功能：瓶身口部区域（身口区）
RtnInfo CCheck::fnSSideFReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	Hlong nNum;
	s_pSSideFReg pSSideFReg = para.value<s_pSSideFReg>();
	s_oSSideFReg oSSideFReg = shape.value<s_oSSideFReg>();
	if (!pSSideFReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	if (pSSideFReg.nShapeType == 0)
	{
		gen_region_contour_xld(oSSideFReg.oCheckRegion,&oSSideFReg.oValidRegion,"filled");
	}
	else
	{
		gen_region_contour_xld(oSSideFReg.oCheckRegion_Rect,&oSSideFReg.oValidRegion,"filled");
	}
	clip_region(oSSideFReg.oValidRegion,&oSSideFReg.oValidRegion,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(oSSideFReg.oValidRegion,&oSSideFReg.oValidRegion,"area","and",100,99999999);
	count_obj(oSSideFReg.oValidRegion,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_SSIDEFIN;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}
	//检测区域预处理
	if (pSSideFReg.bGenRoi)
	{
		s_ROIPara roiPara;
		roiPara.fRoiRatio = pSSideFReg.fRoiRatio;
		roiPara.nClosingWH = pSSideFReg.nClosingWH;
		roiPara.nGapWH = pSSideFReg.nGapWH;
		rtnInfo = genValidROI(imgSrc,roiPara,oSSideFReg.oValidRegion,&oSSideFReg.oValidRegion);
		if (rtnInfo.nType>0)
		{
			return rtnInfo;
		}
	}
	shape.setValue(oSSideFReg);
	//检测爆口
	Hobject imgReduce,imgMean;
	Hobject RegionBody, RegionDynThresh,regExcludeTemp;	
	Hlong nRow11, nCol11, nRow12, nCol12, nValidHei, nValidWidth,nProHei, nProWidth;
	double nRow21, nCol21, nRow22, nCol22;	
	double fRectRatio;
	smallest_rectangle1(oSSideFReg.oValidRegion, &nRow11, &nCol11, &nRow12, &nCol12);
	nValidHei = nRow12-nRow11+1;
	nValidWidth = nCol12-nCol11+1;
	if (pSSideFReg.nShapeType == 0)
	{
		smallest_rectangle1_xld(oSSideFReg.oCheckRegion, &nRow21, &nCol21, &nRow22, &nCol22);
	}
	else
	{
		smallest_rectangle1_xld(oSSideFReg.oCheckRegion_Rect, &nRow21, &nCol21, &nRow22, &nCol22);
	}
	
	nProHei = nRow22-nRow21+1;
	nProWidth = nCol22-nCol21+1;
	double fHeiRatio = nValidHei*1.0/nProHei;
	double fWidRatio = nValidWidth*1.0/nProWidth;	
	rectangularity(oSSideFReg.oValidRegion, &fRectRatio);
	if (fHeiRatio < 0.5 || fWidRatio < 0.5 || fRectRatio < 0.6)//0.67,0.67,0.75[20120725] 2014.6.18：【0.5，0.5，0.7-0.5，0.5，0.6】
	{
		concat_obj(oSSideFReg.oValidRegion,rtnInfo.regError,&rtnInfo.regError);
		rtnInfo.nType = ERROR_SSIDEFIN;
		rtnInfo.strEx = QObject::tr("The actual detection region mistake,or there may be large defects, or wrong location");
		return rtnInfo;		
	}
	// 瓶口区域均值滤波，单独强化水平方向
	reduce_domain(imgSrc,oSSideFReg.oValidRegion,&imgReduce);
	mean_image(imgReduce, &imgMean, pSSideFReg.nMaskSize, pSSideFReg.nMaskSize/5/*2--->5[tyx20120521]*/);
	dyn_threshold(imgReduce, imgMean, &RegionDynThresh,	HTuple(pSSideFReg.nEdge), HTuple("dark"));

	connection(RegionDynThresh, &RegionDynThresh);
	opening_rectangle1(RegionDynThresh, &RegionBody, 1, 5);
	closing_circle(RegionBody, &RegionBody, pSSideFReg.fClosingSize);
	connection(RegionBody, &regExcludeTemp);

	select_shape(regExcludeTemp, &RegionBody, "area", "and", pSSideFReg.nArea, 99999999);
	ExtExcludeDefect(rtnInfo,regExcludeTemp,RegionBody,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,true);
	count_obj(RegionBody,&nNum);
	if (nNum == 0)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject RegSel, RegErr;
	float fOpenSize=pSSideFReg.nWidth;
	gen_empty_obj(&RegErr);
	select_shape(RegionBody, &RegSel, HTuple("phi").Concat("phi"), "or", HTuple(-0.0175*80).Concat(0.0175*55), HTuple(-0.0175*55).Concat(0.0175*80));
	concat_obj(RegErr, RegSel, &RegErr);//斜的(55-80)直接报错

	select_shape(RegionBody, &RegSel, HTuple("phi").Concat("phi"), "or", HTuple(-0.0175*90).Concat(0.0175*80), HTuple(-0.0175*80).Concat(0.0175*90));
	ExtExcludeDefect(rtnInfo,RegionBody,RegSel,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,false);	
	opening_rectangle1(RegSel, &regExcludeTemp, fOpenSize, 1);
	ExtExcludeDefect(rtnInfo,RegSel,regExcludeTemp,EXCLUDE_CAUSE_WIDTH,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,true);	
	select_shape(regExcludeTemp, &RegSel, "area", "and", pSSideFReg.nArea, 99999999);//竖直的(80-90)open下再报错
	ExtExcludeDefect(rtnInfo,regExcludeTemp,RegSel,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,true);	

	concat_obj(RegErr, RegSel, &regExcludeTemp);
	select_shape(regExcludeTemp, &RegErr, "anisometry", "and", 2, 99999);
	ExtExcludeDefect(rtnInfo,regExcludeTemp,RegErr,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,false);	

	int nMouthShadowHei = 20;
	opening_rectangle1(RegionBody, &RegSel, fOpenSize, 1);	
	ExtExcludeDefect(rtnInfo,RegionBody,RegSel,EXCLUDE_CAUSE_WIDTH,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,true);	
	connection(RegSel, &RegSel);
	select_shape(RegSel, &regExcludeTemp, "height", "and", nMouthShadowHei, 99999);
	ExtExcludeDefect(rtnInfo,RegSel,regExcludeTemp,EXCLUDE_CAUSE_HEIGHT,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,false);	
	select_shape(regExcludeTemp, &RegSel, "anisometry", "and", 1, 5);
	ExtExcludeDefect(rtnInfo,regExcludeTemp,RegSel,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,false);	
	//select_shape(RegSel, &RegSel, HTuple("height").Concat("anisometry"), "and",
	//	HTuple(nMouthShadowHei).Concat(1), HTuple(9999).Concat(5));	
	select_shape(RegSel, &regExcludeTemp, HTuple("phi").Concat("phi"), "or", HTuple(-0.0175*91).Concat(0.0175*20), HTuple(-0.0175*20).Concat(0.0175*91));//tyx20120520排除±20°的横向阴影
	ExtExcludeDefect(rtnInfo,RegSel,regExcludeTemp,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,false);	
	select_shape(regExcludeTemp, &RegSel, "area", "and", pSSideFReg.nArea, 99999999);
	ExtExcludeDefect(rtnInfo,regExcludeTemp,RegSel,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,true);	
	concat_obj(RegErr, RegSel, &RegErr);

	Hlong nObjNum, nDstNum, Col1, Col2, width;
	Hobject RegDila, RegRet;
	double GrayOri, GrayDila;//, Aniso, StructFactor, PhiE;
	count_obj(RegErr, &nObjNum);
	if (nObjNum > 0)
	{
		gen_empty_obj(&RegRet);
		for (int i = 0; i < nObjNum; ++i)
		{
			select_obj(RegErr, &RegSel, i+1);

			smallest_rectangle1(RegSel,NULL,&Col1,NULL,&Col2);
			width = Col2-Col1;
			if (width>20) width = 20;
			if(width<5) width = 5;
			dilation_circle(RegSel, &RegDila, width/2+1);

			intensity(RegSel, imgReduce, &GrayOri, NULL);
			intensity(RegDila, imgReduce, &GrayDila, NULL);

			if (GrayDila-GrayOri > 9) 
			{
				concat_obj(RegRet, RegSel, &RegRet);
			}
		}

		count_obj(RegRet, &nDstNum);
		ExtExcludeDefect(rtnInfo,RegErr,RegRet,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideFReg.strName,false);	
		if (nDstNum > 0)
		{
			concat_obj(RegRet,rtnInfo.regError,&rtnInfo.regError);
			rtnInfo.nType = ERROR_SSIDEFIN;			
			return rtnInfo;	
		}			
	}
	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：内口区域
RtnInfo CCheck::fnSInFReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSInFReg pSInFReg = para.value<s_pSInFReg>();
	s_oSInFReg oSInFReg = shape.value<s_oSInFReg>();
	if (!pSInFReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject regValid,regThresh,regBlob,regExcludeTemp;
	Hobject imgReduce;
	Hlong nNum;
	gen_region_contour_xld(oSInFReg.oCheckRegion,&regValid,"filled");
	clip_region(regValid,&regValid,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regValid,&regValid,"area","and",100,99999999);
	count_obj(regValid,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_OVERPRESS;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}
	reduce_domain(imgSrc, regValid, &imgReduce);
	threshold(imgReduce, &regThresh, 0, pSInFReg.nGray);
	opening_circle(regThresh,&regThresh,pSInFReg.fOpeningSize);
	connection(regThresh, &regThresh);
	select_shape(regThresh, &regBlob, HTuple("area"), "or", HTuple(pSInFReg.nArea), HTuple(99999999));
	ExtExcludeDefect(rtnInfo,regThresh,regBlob,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSInFReg.strName,true);
	//select_shape(regBlob,&regBlob,HTuple("compactness").Concat("circularity").Concat("anisometry"),"and",
	//	HTuple(3).Concat(0).Concat(3),HTuple(9999).Concat(0.2).Concat(9999));
	select_shape(regBlob,&regExcludeTemp,HTuple("compactness"),"and",HTuple(3),HTuple(99999));
	ExtExcludeDefect(rtnInfo,regBlob,regExcludeTemp,EXCLUDE_CAUSE_COMPACTNESS,EXCLUDE_TYPE_GENERALDEFEXTS,pSInFReg.strName,false);
	select_shape(regExcludeTemp,&regBlob,HTuple("circularity"),"and",HTuple(0),HTuple(0.2));
	ExtExcludeDefect(rtnInfo,regExcludeTemp,regBlob,EXCLUDE_CAUSE_CIRCULARITY,EXCLUDE_TYPE_GENERALDEFEXTS,pSInFReg.strName,false);
	select_shape(regBlob,&regExcludeTemp,HTuple("anisometry"),"and",HTuple(3),HTuple(99999));
	ExtExcludeDefect(rtnInfo,regBlob,regExcludeTemp,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_GENERALDEFEXTS,pSInFReg.strName,false);

	if (pSInFReg.nPos == 0)
	{
		select_shape(regExcludeTemp, &regBlob, "phi", "and", -0.9, 0);
	}
	else
	{
		select_shape(regExcludeTemp, &regBlob, "phi", "and", 0, 0.9);
	}
	ExtExcludeDefect(rtnInfo,regExcludeTemp,regBlob,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_GENERALDEFEXTS,pSInFReg.strName,false);
	count_obj(regBlob, &nNum);
	if (nNum>0)
	{
		concat_obj(regBlob,rtnInfo.regError,&rtnInfo.regError);
		rtnInfo.nType = ERROR_OVERPRESS;
		return rtnInfo;
	}
	return rtnInfo;
}
//*功能：螺纹口区域
RtnInfo CCheck::fnSScrewFReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSScrewFReg pSScrewFReg = para.value<s_pSScrewFReg>();
	s_oSScrewFReg oSScrewFReg = shape.value<s_oSScrewFReg>();
	if (!pSScrewFReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hlong nNum;
	Hobject imgReduced,imgMean;
	Hobject regValid,regDyn,regConnect,regSel,regExcludeTemp;
	HTuple Row1,Col1,Row2,Col2,Row11,Col11,Row21,Col21;

	if (pSScrewFReg.nShapeType == 0)
	{
		gen_region_contour_xld(oSScrewFReg.oCheckRegion,&regValid,"filled");
	} 
	else
	{
		gen_region_contour_xld(oSScrewFReg.oCheckRegion_Rect,&regValid,"filled");
	}	
	clip_region(regValid,&regValid,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regValid,&regValid,"area","and",100,99999999);
	count_obj(regValid,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_SSCREWF;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	reduce_domain(imgSrc,regValid,&imgReduced);

	mean_image(imgReduced,&imgMean,21,1);
	dyn_threshold(imgReduced,imgMean,&regDyn,pSScrewFReg.nEdge,"dark");
	connection(regDyn,&regConnect);

	//排除靠边区域
	smallest_rectangle1(regValid,NULL,&Col1,NULL,&Col2);
	if (Col2-2>=Col1+2)
	{
		select_shape(regConnect,&regConnect,"column1","and",Col1+2,Col2);
		select_shape(regConnect,&regConnect,"column2","and",Col1,Col2-2);
	}

	//按条件筛选
	select_shape (regConnect, &regExcludeTemp, "area", "and", pSScrewFReg.nArea, 99999999);
	ExtExcludeDefect(rtnInfo,regConnect,regExcludeTemp,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSScrewFReg.strName,true);
	select_shape (regExcludeTemp, &regConnect, "ra", "and", pSScrewFReg.nLength/2.f, 99999);	
	ExtExcludeDefect(rtnInfo,regExcludeTemp,regConnect,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pSScrewFReg.strName,true);
	select_shape(regConnect,&regExcludeTemp,"inner_radius","and",pSScrewFReg.nDia/2.f,99999);
	ExtExcludeDefect(rtnInfo,regConnect,regExcludeTemp,EXCLUDE_CAUSE_INRADIUS,EXCLUDE_TYPE_GENERALDEFEXTS,pSScrewFReg.strName,true);
	select_shape(regExcludeTemp,&regConnect,"anisometry","and",pSScrewFReg.nRab,99999);
	ExtExcludeDefect(rtnInfo,regExcludeTemp,regConnect,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_GENERALDEFEXTS,pSScrewFReg.strName,true);

	select_shape (regConnect, &regExcludeTemp, (HTuple("phi").Append("phi")), 
		"or", (HTuple(-0.0175*90).Append(0.0175*50)), (HTuple(-0.0175*50).Append(0.0175*90)));//角度在[50,90]之间
	ExtExcludeDefect(rtnInfo,regConnect,regExcludeTemp,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_GENERALDEFEXTS,pSScrewFReg.strName,false);

	count_obj(regExcludeTemp,&nNum);
	if (nNum>0)
	{
		Hobject regRect1,regRect2;
		HTuple tMean,tMean1,tMean2;
		Hobject regError;
		gen_empty_obj(&regError);
		for (int i=0;i<nNum;i++)
		{
			select_obj(regExcludeTemp,&regSel,i+1);
			smallest_rectangle1(regSel,&Row1,&Col1,&Row2,&Col2);

			gen_rectangle1(&regRect1,Row1,Col1-5,Row2,Col1);
			gen_rectangle1(&regRect2,Row1,Col2,Row2,Col2+5);

			intensity(regSel,imgSrc,&tMean,NULL);
			intensity(regRect1,imgSrc,&tMean1,NULL);
			intensity(regRect2,imgSrc,&tMean2,NULL);

			if ((tMean<tMean1-pSScrewFReg.nEdge)&&(tMean<tMean2-pSScrewFReg.nEdge)&& ((Row2-Row1)>(Col2-Col1)) )
			{
				concat_obj(regSel,regError,&regError);
			}
		}

		Hlong nNum2 = 0;
		count_obj(regError,&nNum2);
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regError,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pSScrewFReg.strName,true);
		if (nNum2>0)
		{
			concat_obj(regError,rtnInfo.regError,&rtnInfo.regError);
			rtnInfo.nType = ERROR_SSCREWF;						
			return rtnInfo;
		}
	}	
	return rtnInfo;
}


//*功能：瓶口（环形光）内环区域
RtnInfo CCheck::fnFRLInnerReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pFRLInReg pFRLInReg = para.value<s_pFRLInReg>();
	s_oFRLInReg oFRLInReg = shape.value<s_oFRLInReg>();
	if (!pFRLInReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject regCheck,regThresh,regBin,regClose,regOpen,regSelected,regTemp,regFill;
	Hobject imgReduce,imgMean,imgThres,imgTemp,regSelected_2;
	double dInRadius,dOutRadius;
	Hlong nNum;

	smallest_circle(oFRLInReg.oInCircle,NULL,NULL,&dInRadius);
	smallest_circle(oFRLInReg.oOutCircle,NULL,NULL,&dOutRadius);
	if (dOutRadius<dInRadius+5)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_FINISHIN;
		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
		return rtnInfo;		
	}

	difference(oFRLInReg.oOutCircle,oFRLInReg.oInCircle,&regCheck);
	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regCheck,&regCheck,"area","and",100,99999999);
	count_obj(regCheck,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	//threshold(imgSrc, &regTemp, 100, 255);
	//paint_region(regTemp, imgSrc, &imgTemp, 100, "fill");  //2018.1---mj注释，灰度设到100以上有bug
	mean_image(imgSrc,&imgMean,31,31);	
	reduce_domain(imgSrc, regCheck, &imgReduce);
	reduce_domain(imgMean, regCheck, &imgThres);
	dyn_threshold(imgReduce, imgThres, &regThresh, pFRLInReg.nEdge, "light");
	threshold(imgReduce, &regBin, pFRLInReg.nGray, 255);
	union2(regThresh, regBin, &regThresh);

	int i;
	Hobject objSpecial;
	gen_empty_obj(&objSpecial);

	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(regCheck,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regThresh,regDisturb,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
				case 1://典型缺陷				
					intersection(regThresh,regDisturb,&regTemp);
					difference(regThresh,regDisturb,&regThresh);
					opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
					concat_obj(regThresh,regTemp,&regThresh);
					concat_obj(regDisturb,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					break;
				case 2://排除条纹
					{
						Hobject regStripe;
						//2013.9.16 nanjc 增独立参数提取排除
						if (pDistReg.bStripeSelf)
						{
							distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
						}
						else
						{
							intersection(regDisturb,regThresh,&regTemp);
							distRegStripe(pDistReg,regTemp,&regStripe);					
						}
						difference(regThresh,regStripe,&regThresh);
						concat_obj(regStripe,objSpecial,&objSpecial);
						ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					}
					break;
			default:
				break;
			}
		}
	}
	closing_circle(regThresh, &regClose, 3.5);
	opening_circle(regClose,&regOpen,pFRLInReg.fOpenSize);
	//ExtExcludeDefect(rtnInfo,regClose,regOpen,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLInReg.strName,true);	
	difference(regOpen,objSpecial,&regOpen);
	connection(regOpen, &regOpen);
	select_shape(regOpen,&regSelected, "area","and", pFRLInReg.nArea, 99999999);
	ExtExcludeDefect(rtnInfo,regOpen,regSelected,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLInReg.strName,true);
	select_shape(regSelected,&regSelected_2, "inner_radius","and", pFRLInReg.nInnerRadius/2, 99999);  //2017.3.21-内接圆改为设直径，为了与缺陷提示信息匹配
	ExtExcludeDefect(rtnInfo,regSelected,regSelected_2,EXCLUDE_CAUSE_INRADIUS,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLInReg.strName,true);

	count_obj(regSelected_2, &nNum);
	if (nNum > 0)
	{
		concat_obj(rtnInfo.regError,regSelected_2,&rtnInfo.regError);
		rtnInfo.nType = ERROR_FINISHIN;
		return rtnInfo;
	}

	//2014.3.3	// 双口检测
	//极坐标系下分割口平面区域（20130328由全图极坐标变换改进为扩展后的口平面区域变换，防止内外环报剪刀印）
	//if (pFRLInReg.bOverPress)
	//{
	//	Hobject imgPolar,imgPlMean,regSpecialPolar;
	//	double dOriRow = currentOri.Row;
	//	double dOriCol = currentOri.Col;
	//	int nExpand = 10;
	//	double nDist = dOutRadius-dInRadius+2*nExpand;
	//	polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
	//		dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
	//	mean_image(imgPolar,&imgPlMean,31,5);
	//	dyn_threshold(imgPolar,imgPlMean,&regThresh,pFRLInReg.nPressEdge,"light");

	//	polar_trans_region_inv(regThresh,&regThresh,dOriRow,dOriCol,0,2*PI,
	//		dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
	//	concat_obj(rtnInfo.regError,regThresh,&rtnInfo.regError);
	//	rtnInfo.nType = ERROR_OVERPRESS;
	//	return rtnInfo;
	//}

	//双口检测
	if (pFRLInReg.bOverPress)
	{
		dyn_threshold(imgReduce,imgThres,&regThresh,pFRLInReg.nPressEdge,"light");
		difference(regThresh,objSpecial,&regThresh);

		//2014.3.3 预选+闭运算，防止很小提取断环
		connection(regThresh, &regThresh);
		select_shape(regThresh, &regThresh, HTuple("rb"), "and",HTuple(dInRadius/3), HTuple(99999));
		union1(regThresh, &regThresh);
		closing_circle(regThresh, &regThresh,2.5);
		//////////////////////////////////////////////////////////////////////////

		connection(regThresh, &regThresh);
		fill_up(regThresh, &regFill);		
		select_shape(regFill, &regSelected, HTuple("rb"), "and",HTuple(dInRadius), HTuple(99999));


		//concat_obj(rtnInfo.regError,regThresh,&rtnInfo.regError);
		//rtnInfo.nType = ERROR_OVERPRESS;

		count_obj(regSelected, &nNum);
		if (nNum == 0)
		{
			//20120829此处不报错
		}
		else if (nNum >= pFRLInReg.nPressNum)
		{
			concat_obj(rtnInfo.regError,regSelected,&rtnInfo.regError);
			rtnInfo.nType = ERROR_OVERPRESS;
			return rtnInfo;
		}
		//2014.1.15 针对双口部分区域连在一起的情况，添加如下判断
		else
		{
			intersection(regThresh,regSelected,&regThresh);
			union1(regThresh,&regThresh);
			closing_circle(regThresh,&regClose,6.5);
			difference(regClose,regThresh,&regThresh);
			connection(regThresh,&regThresh);
			//2014.3.3 dInRadius ->dInRadius/2
			select_shape(regThresh, &regSelected, HTuple("rb"), "and",HTuple(dInRadius/2), HTuple(99999));
			count_obj(regSelected, &nNum);
			if (nNum+1>= pFRLInReg.nPressNum)
			{
				concat_obj(rtnInfo.regError,regSelected,&rtnInfo.regError);
				rtnInfo.nType = ERROR_OVERPRESS;
				return rtnInfo;
			}
		}
	}

	//内环破损检测
	if (pFRLInReg.bInBroken)
	{
		double dCircu;
		Hobject BorderSkeleton, regDiff;
		dyn_threshold(imgReduce,imgThres,&regThresh,pFRLInReg.nBrokenEdge,"light");
		difference(regThresh,objSpecial,&regThresh);
		connection(regThresh, &regThresh);
		select_shape(regThresh, &regThresh, "area", "and", 30, 99999999);
		count_obj(regThresh, &nNum);
		if (nNum > 0)
		{
			select_shape_std(regThresh, &regTemp, "max_area", 70);
			fill_up(regTemp, &regTemp);
			circularity(regTemp, &dCircu);
			if (dCircu < 0.3)
			{
				union1(regThresh, &regThresh);

				closing_circle(regThresh, &regThresh, dOutRadius-dInRadius);
				connection(regThresh, &regThresh);
				select_shape(regThresh, &regThresh, HTuple("outer_radius"), "and", HTuple(dInRadius), HTuple(9999));
				count_obj(regThresh, &nNum);
				if (nNum > 0)
				{
					select_shape_std(regThresh, &regThresh, "max_area", 70);
					fill_up(regThresh, &regThresh);

					circularity(regThresh, &dCircu);
					if (dCircu > 0.7)
					{
						opening_circle(regThresh, &BorderSkeleton, 75.5);
						difference(regThresh, BorderSkeleton, &regDiff);
						connection(regDiff, &regDiff);
						select_shape(regDiff, &regSelected, "area", "and", pFRLInReg.nBrokenArea, 99999999);
						ExtExcludeDefect(rtnInfo,regDiff,regSelected,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLInReg.strName,true);
						count_obj(regSelected, &nNum);
						if (nNum > 0)
						{
							double dGrayMean, dDevi;
							Hobject regErr;
							Hlong nErrNum = 0;
							gen_empty_obj(&regErr);
							for (int i = 0; i<nNum; ++i)
							{
								select_obj(regSelected, &regTemp, i+1);
								dilation_circle(regTemp, &regTemp, 5.5);
								intensity(regTemp, imgSrc, &dGrayMean, &dDevi);
								if (dGrayMean > pFRLInReg.nBrokenGrayMean)
								{
									concat_obj(regErr, regTemp, &regErr);
									nErrNum += 1;
								}
							}
							if (nErrNum > 0)
							{
								concat_obj(rtnInfo.regError,regErr,&rtnInfo.regError);
								rtnInfo.nType = ERROR_FINISHIN;						
								return rtnInfo;									
							}							
						}
					}
				}
			}
		}

		//closing_circle(regThresh, &regThresh, 5.5);
		//connection(regThresh, &regThresh);
		//select_shape(regThresh, &regThresh, HTuple("outer_radius"), "and", HTuple(dInRadius), HTuple(9999));
		//count_obj(regThresh, &nNum);
		//if (nNum > 0)
		//{
		//	select_shape_std(regThresh, &regThresh, "max_area", 70);
		//	fill_up(regThresh, &regThresh);
		//	double dCircu;
		//	Hobject BorderSkeleton, regDiff;
		//	circularity(regThresh, &dCircu);
		//	if (dCircu < 0.5)
		//	{
		//		skeleton(regThresh, &BorderSkeleton);
		//		shape_trans(BorderSkeleton, &BorderSkeleton, "convex");
		//		boundary(BorderSkeleton, &BorderSkeleton, "inner");
		//		difference(BorderSkeleton, regThresh, &regDiff);
		//		connection(regDiff, &regDiff);
		//		select_shape(regDiff, &regSelected, "contlength", "and", pFRLInReg.fBrokenSize*2, 99999999);
		//		ExtExcludeDefect(rtnInfo,regDiff,regSelected,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_BROKENRING,pFRLInReg.strName,true);
		//		count_obj(regSelected, &nNum);
		//		if (nNum > 0)
		//		{
		//			concat_obj(rtnInfo.regError,regDiff,&rtnInfo.regError);
		//			rtnInfo.nType = ERROR_FINISHIN;						
		//			return rtnInfo;	
		//		}
		//	}
		//}
	}

	return rtnInfo;
}

// 2017.12更新---增加剪刀印检测新算法
//*功能：环形光口平面区域---2017.3修改
RtnInfo CCheck::fnFRLMiddleReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pFRLMidReg pFRLMidReg = para.value<s_pFRLMidReg>();
	s_oFRLMidReg oFRLMidReg = shape.value<s_oFRLMidReg>();
	if (!pFRLMidReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double dInRadius,dOutRadius;
	Hobject regCheck,regThresh,regBin,regConnect,regSelect,regExcludeTemp;
	Hobject reThres1, regThres2;
	Hobject imgReduce,imgMean;
	Hlong nNum;
	int i;
	smallest_circle(oFRLMidReg.oInCircle,NULL,NULL,&dInRadius);
	smallest_circle(oFRLMidReg.oOutCircle,NULL,NULL,&dOutRadius);
	if (dOutRadius<dInRadius+5)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_FINISHMID;
		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
		return rtnInfo;		
	}
	difference(oFRLMidReg.oOutCircle, oFRLMidReg.oInCircle, &regCheck);	
	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regCheck,&regCheck,"area","and",100,99999999);
	count_obj(regCheck,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	mean_image(imgSrc,&imgMean,31,31);
	reduce_domain(imgSrc, regCheck, &imgReduce);
	reduce_domain(imgMean, regCheck, &imgMean);
	// 使用第一重条件提取
	dyn_threshold(imgReduce, imgMean, &regThresh, pFRLMidReg.nEdge, "light");
	threshold(imgReduce, &regBin, pFRLMidReg.nGray, 255);
	union2(regThresh, regBin, &regThresh);
	if (pFRLMidReg.bOpen)
	{
		copy_obj(regThresh,&regExcludeTemp,1,-1);
		opening_circle(regThresh, &regThresh, pFRLMidReg.fOpenSize);
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
	}
	connection(regThresh, &regConnect);
	select_shape(regConnect, &regSelect, HTuple("area").Concat("ra"), "or", 
		HTuple(pFRLMidReg.nArea).Concat(pFRLMidReg.nLen/2), HTuple(99999999).Concat(99999));
	ExtExcludeDefect(rtnInfo,regConnect,regSelect,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
	copy_obj (regSelect, &reThres1, 1, -1);

	// 使用第二重条件提取
	if (pFRLMidReg.nEdge_2!=pFRLMidReg.nEdge || pFRLMidReg.nGray_2!=pFRLMidReg.nGray)
	{
		dyn_threshold(imgReduce, imgMean, &regThresh, pFRLMidReg.nEdge_2, "light");
		threshold(imgReduce, &regBin, pFRLMidReg.nGray_2, 255);
		union2(regThresh, regBin, &regThresh);
		if (pFRLMidReg.bOpen)
		{
			copy_obj(regThresh,&regExcludeTemp,1,-1);
			opening_circle(regThresh, &regThresh, pFRLMidReg.fOpenSize);
			ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
		}
		connection(regThresh, &regConnect);
		select_shape(regConnect, &regSelect, HTuple("area").Concat("ra"), "or", 
			HTuple(pFRLMidReg.nArea_2).Concat(pFRLMidReg.nLen_2/2), HTuple(99999999).Concat(99999));
		ExtExcludeDefect(rtnInfo,regConnect,regSelect,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
		copy_obj (regSelect, &regThres2, 1, -1);
	}

	union2(reThres1, regThres2, &regThresh);

	Hobject objSpecial,regSpecialPolar,regTemp;
	gen_empty_obj(&objSpecial);
	gen_empty_obj(&regSpecialPolar);
	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(regCheck,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regThresh,regDisturb,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
			case 1://典型缺陷				
				intersection(regThresh,regDisturb,&regTemp);
				difference(regThresh,regDisturb,&regThresh);
				opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
				concat_obj(regThresh,regTemp,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				break;
			case 2://排除条纹
				{
					Hobject regStripe;
					//2013.9.16 nanjc 增独立参数提取排除
					if (pDistReg.bStripeSelf)
					{
						distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
					}
					else
					{
						intersection(regDisturb,regThresh,&regTemp);
						distRegStripe(pDistReg,regTemp,&regStripe);					
					}
					difference(regThresh,regStripe,&regThresh);
					concat_obj(regStripe,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				}
				break;
			default:
				break;
			}
		}
	}

	connection(regThresh, &regConnect);
	select_shape(regConnect, &regSelect, "area", "and", 1, 99999999);
	count_obj(regSelect, &nNum);
	if (nNum > 0)
	{
		concat_obj(rtnInfo.regError,regSelect,&rtnInfo.regError);
		rtnInfo.nType = ERROR_FINISHMID;
		return rtnInfo;
	}

	// 检测口麻 [5/25/2012]
	if (pFRLMidReg.bPitting)
	{
		dyn_threshold(imgReduce, imgMean, &regThresh, pFRLMidReg.nPitEdge, "light");
		connection(regThresh, &regConnect);
		select_shape(regConnect, &regSelect, "area", "and", pFRLMidReg.nPitArea, 99999999);
		ExtExcludeDefect(rtnInfo,regConnect,regSelect,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_PITTING,pFRLMidReg.strName,true);	
		select_shape(regSelect, &regConnect, "circularity", "and", 0.3, 1);
		ExtExcludeDefect(rtnInfo,regSelect,regConnect,EXCLUDE_CAUSE_CIRCULARITY,EXCLUDE_TYPE_PITTING,pFRLMidReg.strName,true);	
		count_obj(regConnect, &nNum);
		if (nNum >= pFRLMidReg.nPitNum)
		{
			concat_obj(rtnInfo.regError,regSelect,&rtnInfo.regError);
			rtnInfo.nType = ERROR_PITTING;
			return rtnInfo;
		}
	}

	// 检测剪刀印 [2017.12.28更新-MJ] 
	if (pFRLMidReg.bLOF)
	{
		if (pFRLMidReg.bLOFOldWay)
		{
			Hobject imgPolar,imgPlMean,imgCentMean,imgPlReduce,imgReduce2;
			Hobject regDyn,regConn,regSel,regUnion,rectDomain,regDiff,
				regClose,regTemp,regBin,regBinTemp,regBinBorder,regBoder,boder1,boder2;
			Hobject ErrorObj,ErrorObjBorder,ErrorObjFinal;
			Hlong row1,col1,row2,col2,row11,row21;		
			double minDist,minDist1,minDist2;
			int nExpand = 50;//内圈、外圈向内外扩展的宽度 Q:50->10（口平面宽度较小时，50会影响检测）
			double nDist = dOutRadius-dInRadius+2*nExpand;
			dLOFHei = nDist;
			double dOriRow = currentOri.Row;
			double dOriCol = currentOri.Col;
			int nLOFLen = pFRLMidReg.nLOFLen; //剪刀印长度
			float fLOFRab = pFRLMidReg.fLOFRab;
			int nLOFLen_In = pFRLMidReg.nLOFLen_In;  //靠里环剪刀印长度
			float fLOFRab_In = pFRLMidReg.fLOFRab_In;
			int nLOFLen_Out = pFRLMidReg.nLOFLen_Out;  //靠外环剪刀印长度
			float fLOFRab_Out = pFRLMidReg.fLOFRab_Out;
			int nDiaMin = pFRLMidReg.nLOFDiaMin; //剪刀印内接圆直径下限
			int nDiaMax = pFRLMidReg.nLOFDiaMax; //剪刀印内接圆直径上限
			int nAngleOffset = pFRLMidReg.nLOFAngleOffset; //两个剪刀印夹角
			
			//极坐标系下分割口平面区域（20130328由全图极坐标变换改进为扩展后的口平面区域变换，防止内外环报剪刀印）
			polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
			polar_trans_region(objSpecial,&regSpecialPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
			mean_image(imgPolar,&imgPlMean,31,31);
			dyn_threshold(imgPolar,imgPlMean,&regDyn,5,"light");
			difference(regDyn,regSpecialPolar,&regDyn);
			//20130508 防止口面灰尘太多，影响口平面区域提取
			connection(regDyn,&regDyn);
			select_shape(regDyn,&regDyn,"area","and",10,99999999);
			union1(regDyn,&regDyn);
			//
			opening_rectangle1 (regDyn, &regDyn, 3, 1);//2013.12.30 开掉麻点的影响
			closing_rectangle1(regDyn,&regDyn,30,1);//20130124新增//20130226 10-->30 防止大缺口造成内外环断裂//20130715 10-->1,天马
			difference(regDyn,regSpecialPolar,&regDyn);
			connection(regDyn,&regDyn);
			select_shape(regDyn,&regDyn,HTuple("anisometry").Concat("phi"),"and",HTuple(5).Concat(-0.35),
				HTuple(9999).Concat(0.35));//20130703选择长宽比>5，且±20°的横向区域,防止南充内口太浅断开
			union1(regDyn,&regDyn);
			closing_rectangle1(regDyn,&regDyn,m_nWidth/3,2);//将最大1/3图像宽度的缺口填补
			difference(regDyn,regSpecialPolar,&regDyn);
			//select_shape(regDyn,&regDyn,"width","and",m_nWidth/5,99999);//避免口面太脏造成影响
			//closing_rectangle1(regDyn,&regDyn,50,2);//内/外环断开时防止分割错误
			connection(regDyn,&regConn);
			select_shape(regConn,&regSel,"width","and",m_nWidth*0.9,9999);//0.9天马
			union1(regSel,&regUnion);
			count_obj(regUnion,&nNum);
			if (nNum==0)
			{
				return rtnInfo;
			}
			smallest_rectangle1(regUnion,&row1,&col1,&row2,&col2);
			gen_rectangle1(&rectDomain,row1,col1,row2,col2);
			reduce_domain(imgPolar,rectDomain,&imgPlReduce);
			difference(rectDomain,regUnion,&regDiff);	//2013.9.18 nanjc 修改：regDyn->regUnion，中环略亮干扰，报边界异常
			closing_rectangle1(regDiff,&regClose,150,1);//剪刀印最大可检宽度为150
			difference(regClose,regSpecialPolar,&regClose);
			opening_rectangle1(regClose,&regClose,150,1);//20130124内环太浅时有问题
			connection(regClose,&regClose);	
			//	select_shape_std(regClose,&regSel,"max_area",70);	//20121217
			select_shape(regClose,&regSel,
				HTuple("width").Concat("height").Concat("rectangularity").Concat("area_holes"),"and",
				HTuple(m_nWidth*0.9).Concat(nDist*0.5).Concat(0.7).Concat(0),
				HTuple(m_nWidth*1.1).Concat(nDist).Concat(1.0).Concat(10));
			count_obj(regSel,&nNum);
			if (nNum==0)
			{			
				select_shape_std(regClose,&regSel,"max_area",70);			
			} else if(nNum > 1)
			{
				select_shape_std(regSel,&regSel,"max_area",70);//20121228
			}
			union1(regSel,&regSel);		
			dilation_rectangle1(regSel,&regSel,1,3);//上下扩展3像素，检测更全一些
			erosion_rectangle1(regSel,&regSel,1,5);
			closing_rectangle1 (regSel,&regSel, 1, 20);//内外环中间弧形油污影响
			fill_up(regSel,&regSel);
			//生成上下边界，用于增强靠边剪刀印
			boundary(regSel,&regBoder,"inner");
			clip_region(regBoder,&regBoder,0,1,m_nHeight,m_nWidth-2);
			connection(regBoder,&regBoder);
			count_obj(regBoder,&nNum);
			if (nNum!=2)
			{
				//2015.1.28肇庆通产去掉该项错误
				//gen_rectangle1(&rtnInfo.regError,120,20,220,120);
				//rtnInfo.nType = ERROR_FINISHMID;
				//rtnInfo.strEx = QObject::tr("The inner ring or the outer ring too small to cause border error when detecting scissors.");
				return rtnInfo;	
			}
			sort_region (regBoder, &regBoder, "first_point", "true", "row");
			select_obj(regBoder,&boder1,1);
			select_obj(regBoder,&boder2,2);
			///计算口平面区域宽度
			gen_rectangle1(&regTemp,row1,m_nWidth/2-5,row2,m_nWidth/2+5);
			intersection(regTemp,regSel,&regTemp);
			smallest_rectangle1(regTemp,&row11,NULL,&row21,NULL);
			float fLength=row21-row11;
			if (fLength<(dOutRadius-dInRadius)/2.f)
			{
				//gen_rectangle1(&rtnInfo.regError,120,20,220,120);
				//rtnInfo.nType = ERROR_FINISHMID;
				//rtnInfo.strEx = QObject::tr("The inner ring or the outer ring too small to cause width error when detecting scissors.");
				//return rtnInfo;	
			}
			else
			{
				reduce_domain(imgPlReduce,regSel,&imgReduce2);
				//检测剪刀印
				mean_image(imgReduce2,&imgCentMean,150,1);//横向强化平滑
				dyn_threshold(imgReduce2,imgCentMean,&regDyn,pFRLMidReg.nLOFEdge,"light");
				connection(regDyn,&regDyn);
			
				select_shape(regDyn,&regDyn,"area","and",3,99999999);
				union1(regDyn,&regDyn);
				//2015.1.27 肇庆闭运算小于1Bug
				closing_rectangle1(regDyn,&regDyn,1,max(1,fLength/10));//间隙小于1/10封盖面宽度的，被合并
				difference(regDyn,regSpecialPolar,&regDyn);
				opening_rectangle1(regDyn,&regDyn,1,2);//20130124内环有可能存在横向连接
				connection(regDyn,&regBin);
				select_shape(regBin,&regBin,"area","and",1,99999);//20130624防止面积为0的空区域造成异常
				count_obj(regBin,&nNum);
				if (nNum==0)
				{
					return rtnInfo;
				}
			
				//1.从中选出靠边剪刀印，用更小的长度单独检测
				//2017.3增加---靠内环和靠外环的剪刀印提取标准不同
				int minLOFLen2;
				minLOFLen2 = min(nLOFLen_In, nLOFLen_Out);
				select_shape(regBin, &regBinTemp, "height", "and", fLength*minLOFLen2/100, 99999);
				count_obj(regBinTemp, &nNum);
				if (nNum==0)
				{
					return rtnInfo;
				}
			
				gen_empty_obj(&ErrorObjBorder);		
				for(i=1;i<=nNum;i++)
				{
					select_obj(regBinTemp, &regSel, i);
					distance_rr_min(regSel, boder1, &minDist1, NULL, NULL, NULL, NULL);
					distance_rr_min(regSel, boder2, &minDist2, NULL, NULL, NULL, NULL);
			        minDist = min(minDist1,minDist2);
					if (minDist<2) //靠近边界的距离小于2时，认为是边界剪刀印
					{
						smallest_rectangle1(regSel, &row1, &col1, &row2, &col2);
						col2=col2==col1?col2+1:col2;//防止极细的剪刀印宽度为0
						float mouthTLRab=(float)(row2-row1)/(float)(col2-col1);
						float mouthTLHeight=(float)row2-row1;   
						// 区分靠里剪刀印与靠外剪刀印
						if(minDist == minDist1) //靠里剪刀印 
						{
							if (mouthTLRab>fLOFRab_In && mouthTLHeight>fLength*nLOFLen_In/100)
							{
								concat_obj(regSel, ErrorObjBorder, &ErrorObjBorder);
							}
						}
						else //靠外剪刀印
						{
							if (mouthTLRab>fLOFRab_Out && mouthTLHeight>fLength*nLOFLen_Out/100)
							{
								concat_obj(regSel, ErrorObjBorder, &ErrorObjBorder);
							}
			
						}
					}
				}
			
				//2. 检测整体的剪刀印
				select_shape(regBin,&regBin,"height","and",fLength*nLOFLen/100,99999);
				//此处不提取排除缺陷，否则由于剪刀印对比度要求较低，提取过多干扰
				//copy_obj(regBin,&regExcludeTemp,1,-1);
				//select_shape(regBin,&regBin,"height","and",fLength*nLOFLen/100,99999);
				//ExtExcludePolarDefect(rtnInfo,regExcludeTemp,regBin,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_LOF,pFRLMidReg.strName,true);		
			
				gen_empty_obj(&ErrorObj);		
				count_obj(regBin,&nNum);
				for (i=1;i<nNum+1;i++)
				{			
					select_obj(regBin,&regSel,i);			
					smallest_rectangle1(regSel,&row1,&col1,&row2,&col2);	
					col2=col2==col1?col2+1:col2;
					float mouthTLRab=(float)(row2-row1)/(float)(col2-col1);
					if (mouthTLRab>fLOFRab)
					{						
						concat_obj(regSel,ErrorObj,&ErrorObj);
					}
				}	
				ExtExcludePolarDefect(rtnInfo,regBin,ErrorObj,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_LOF,pFRLMidReg.strName,true);				
				concat_obj(ErrorObj,ErrorObjBorder,&ErrorObj);
				union1(ErrorObj,&ErrorObj);
				connection(ErrorObj,&ErrorObj);
			
				count_obj(ErrorObj,&nNum);
				if (nNum>0)
				{	
					polar_trans_region_inv(ErrorObj,&ErrorObj,dOriRow,dOriCol,0,2*PI,
						dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
			
					// 2017.3.20添加-宽度判断，防止其他白色缺陷干扰
					nDiaMin = nDiaMin<nDiaMax?nDiaMin:nDiaMax;
					select_shape (ErrorObj, &ErrorObjFinal, "inner_radius", "and", nDiaMin/2, nDiaMax/2);
					ExtExcludeDefect(rtnInfo,ErrorObj,ErrorObjFinal,EXCLUDE_CAUSE_INRADIUS,EXCLUDE_TYPE_LOF,pFRLMidReg.strName);
					count_obj (ErrorObjFinal, &nNum);
					if(nNum > 0)
					{
							if (nNum == 2) //2017.3添加-判断两条跨口线间的夹角，如果为180度，认为是模缝线非缺陷
							{
								HTuple ErrRow,ErrCol,Angle,Degree;
								double degree;
								area_center (ErrorObjFinal, NULL, &ErrRow, &ErrCol);
								angle_ll (dOriRow,dOriCol,ErrRow[0],ErrCol[0],dOriRow,dOriCol,ErrRow[1],ErrCol[1],&Angle);
								Degree = Angle.Deg();
						        degree = fabs(Degree[0].D());
								if (fabs(degree-180) < nAngleOffset)
								{
									//模缝线
								}
							}  
							else
							{
								concat_obj(rtnInfo.regError, ErrorObjFinal, &rtnInfo.regError);
								rtnInfo.nType = ERROR_LOF;
								return rtnInfo;
							}
					}	
				}	
			}
		}
		if (pFRLMidReg.bLOFNewWay)
		{
			Hobject imgPolar,imgPlMean,/*imgCentMean,*/imgPlReduce,imgReduce2;
			Hobject regDyn,regConn,regSel,regUnion,rectDomain,regDiff,regClose;
			Hlong row1,col1,row2,col2,row11,row21;		
			int nExpand = 50;//内圈、外圈向内外扩展的宽度
			double nDist = dOutRadius-dInRadius+2*nExpand;
			dLOFHei = nDist;
			double dOriRow = currentOri.Row;
			double dOriCol = currentOri.Col;
			int nEdge = pFRLMidReg.nLOFEdge_new;    //剪刀印对比度
			int nWidth = pFRLMidReg.nLOFWidth_new;  //剪刀印宽度
			int nLen = pFRLMidReg.nLOFLen_new;      //剪刀印长度（比例）

			//极坐标系下分割口平面区域（20130328由全图极坐标变换改进为扩展后的口平面区域变换，防止内外环报剪刀印）
			polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
			polar_trans_region(objSpecial,&regSpecialPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
			mean_image(imgPolar,&imgPlMean,31,31);
			dyn_threshold(imgPolar,imgPlMean,&regDyn,5,"light");
			difference(regDyn,regSpecialPolar,&regDyn);
			//20130508 防止口面灰尘太多，影响口平面区域提取
			connection(regDyn,&regDyn);
			select_shape(regDyn,&regDyn,"area","and",10,99999999);
			union1(regDyn,&regDyn);
			opening_rectangle1 (regDyn, &regDyn, 3, 1);//2013.12.30 开掉麻点的影响
			closing_rectangle1(regDyn,&regDyn,30,1);//20130124新增//20130226 10-->30 防止大缺口造成内外环断裂//20130715 10-->1,天马
			difference(regDyn,regSpecialPolar,&regDyn);
			connection(regDyn,&regDyn);
			select_shape(regDyn,&regDyn,HTuple("anisometry").Concat("phi"),"and",HTuple(5).Concat(-0.35),
				HTuple(9999).Concat(0.35));//20130703选择长宽比>5，且±20°的横向区域,防止南充内口太浅断开
			union1(regDyn,&regDyn);
			closing_rectangle1(regDyn,&regDyn,m_nWidth/3,2);//将最大1/3图像宽度的缺口填补
			difference(regDyn,regSpecialPolar,&regDyn);
			//select_shape(regDyn,&regDyn,"width","and",m_nWidth/5,99999);//避免口面太脏造成影响
			//closing_rectangle1(regDyn,&regDyn,50,2);//内/外环断开时防止分割错误
			connection(regDyn,&regConn);
			select_shape(regConn,&regSel,"width","and",m_nWidth*0.9,9999);//0.9天马
			union1(regSel,&regUnion);
			count_obj(regUnion,&nNum);
			if (nNum==0)
			{
				return rtnInfo;
			}
			smallest_rectangle1(regUnion,&row1,&col1,&row2,&col2);
			gen_rectangle1(&rectDomain,row1,col1,row2,col2);
			reduce_domain(imgPolar,rectDomain,&imgPlReduce);
			difference(rectDomain,regUnion,&regDiff);	//2013.9.18 nanjc 修改：regDyn->regUnion，中环略亮干扰，报边界异常
			closing_rectangle1(regDiff,&regClose,150,1);//剪刀印最大可检宽度为150
			difference(regClose,regSpecialPolar,&regClose);
			opening_rectangle1(regClose,&regClose,150,1);//20130124内环太浅时有问题
			connection(regClose,&regClose);	
			count_obj (regClose, &nNum);
			if (nNum==0)
			{
				return rtnInfo;
			}

			select_shape(regClose,&regSel,"width","and",m_nWidth*0.9,m_nWidth*1.1);
			count_obj(regSel,&nNum);
			//判断是否包含中心点
			Hobject regCandi;
			gen_empty_obj(&regCandi);
			for (int i=1;i<=nNum;i++)
			{
				Hlong isInside;
				select_obj (regSel,&regTemp,i);
				test_region_point(regTemp, nDist/2, m_nWidth/2, &isInside);
				if (isInside == 1)
				{
					concat_obj (regCandi,regTemp,&regCandi);
				}
			}
			count_obj (regCandi,&nNum);
			if (nNum == 0)
			{
				return rtnInfo;
			}
			else
			{
				select_shape_std(regCandi, &regSel, "max_area", 70);
			}
			union1(regSel,&regSel);		
			dilation_rectangle1(regSel,&regSel,1,3);//上下扩展3像素，检测更全一些
			erosion_rectangle1(regSel,&regSel,1,5);
			closing_rectangle1 (regSel,&regSel, 1, 20);//内外环中间弧形油污影响
			fill_up(regSel,&regSel);

			///计算口平面区域宽度
			gen_rectangle1(&regTemp,row1,m_nWidth/2-5,row2,m_nWidth/2+5);
			intersection(regTemp,regSel,&regTemp);
			smallest_rectangle1(regTemp,&row11,NULL,&row21,NULL);
			float fLength=row21-row11;
			if (fLength<(dOutRadius-dInRadius)/2.f)
			{
				//gen_rectangle1(&rtnInfo.regError,120,20,220,120);
				//rtnInfo.nType = ERROR_FINISHMID;
				//rtnInfo.strEx = QObject::tr("The inner ring or the outer ring too small to cause width error when detecting scissors.");
				//return rtnInfo;	
			}
			else
			{
				Hobject ImageResult,ImageResult1,Lines,LinesTemp;
				Hobject LineReg,LineUnion,LineClo,LineCon,LineRect,RectDila,regPolar;
				reduce_domain(imgPlReduce,regSel,&imgReduce2);
				//检测剪刀印
				mean_image(imgReduce2,&imgMean,5,5);
				HTuple hv_filter;
				hv_filter=HTuple();
				hv_filter[0] = 4;
				hv_filter[1] = 4;
				hv_filter[2] = 8;
				hv_filter[3] = -1;
				hv_filter[4] = 2;
				hv_filter[5] = 2;
				hv_filter[6] = -1;
				hv_filter[7] = -1;
				hv_filter[8] = 2;
				hv_filter[9] = 2;
				hv_filter[10] = -1;
				hv_filter[11] = -1;
				hv_filter[12] = 2;
				hv_filter[13] = 2;
				hv_filter[14] = -1;
				hv_filter[15] = -1;
				hv_filter[16] = 2;
				hv_filter[17] = 2;
				hv_filter[18] = -1;
				convol_image (imgMean, &ImageResult, hv_filter, "mirrored");
				gray_range_rect (ImageResult, &ImageResult1, 1, 5);
				HTuple Sigma,Low,High;
				calculate_lines_gauss_parameters (nWidth, nEdge, &Sigma, &Low, &High); 
				lines_gauss (ImageResult1, &Lines, Sigma, Low, High, "light", "true", "bar-shaped", "true");
				// 用长宽比、长度、phi值来筛选---待修改：添加排除缺陷信息  
				select_shape_xld (Lines, &LinesTemp, HTuple("anisometry").Concat("anisometry"), "or", HTuple(2).Concat(0), HTuple(999).Concat(0));	
				select_shape_xld (LinesTemp, &LinesTemp, "height", "and", fLength*nLen/100, 99999);
				select_shape_xld (LinesTemp, &LinesTemp, HTuple("rect2_phi").Concat("rect2_phi"), "or", HTuple(-1.77).Concat(1.37), HTuple(-1.37).Concat(1.77));
				count_obj (LinesTemp, &nNum);
				if (nNum == 0)
				{
					return rtnInfo;
				}
				else
				{
					gen_region_contour_xld (LinesTemp, &LineReg, "filled");
					union1 (LineReg, &LineUnion);
					closing_rectangle1 (LineUnion, &LineClo, 30, 1);
					connection (LineClo, &LineCon);
					shape_trans (LineCon, &LineRect,"rectangle1");
					dilation_rectangle1 (LineRect, &RectDila, 10, 1);
					polar_trans_region_inv(RectDila, &regPolar, dOriRow, dOriCol, 0, 2*PI,dInRadius-nExpand, dOutRadius+nExpand, m_nWidth, nDist, m_nWidth, m_nHeight, "nearest_neighbor");
					concat_obj(rtnInfo.regError, regPolar, &rtnInfo.regError);
					rtnInfo.nType = ERROR_LOF;
					return rtnInfo;
				}		
			}	
		}			
	}

	//2013.11.2 索坤口变形,从C++中移植过来 nDeformWidth 改动 100--》150
	//检测瓶口变形 [7/11/2013 Nanjc] 
	//定义错误类型
	if(pFRLMidReg.bDeform)
	{
		Hobject imgPolar,imgPlMean,imgCentMean,imgPlReduce,imgReduce2;
		Hobject regDyn,regConn,regSel,regSel1,regClosing,regDifference;
		Hobject ErrorObj,ErrorObjBorder;
		Hlong Row1,Column1,Row2,Column2;
		double CenRow1,CenCol1,CenRow2,CenCol2;
		HTuple Rows, Cols,Length;
		HTuple tupleRowPtNum,tupleCloumnMin,tupleCloumnMax,tupleUpOrDown,tupleKeystone;
		double dGrayMean;
		int nExpand = 35;//内圈、外圈向内外扩展的宽度*加宽，为将两个边缘都扩进来
		double nDist = dOutRadius-dInRadius+2*nExpand;

		double dOriRow = currentOri.Row;
		double dOriCol = currentOri.Col;
		int nDeformGary = pFRLMidReg.nDeformGary;
		int nDeformHei = pFRLMidReg.nDeformHei;
		int nDeformCirWid = pFRLMidReg.nDeformCirWid;

		int nDeformWidth = 150;
		//极坐标系下分割口平面区域（20130328由全图极坐标变换改进为扩展后的口平面区域变换，防止内外环报剪刀印）

		polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		polar_trans_region(objSpecial,&regSpecialPolar,dOriRow,dOriCol,0,2*PI,
			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		mean_image(imgPolar,&imgPlMean,31,31);
		dyn_threshold(imgPolar,imgPlMean,&regDyn,5,"light");		
		difference(regDyn,regSpecialPolar,&regDyn);

		opening_rectangle1 (regDyn, &regDyn, 3, 1);//开掉麻点的影响
		closing_rectangle1 (regDyn, &regDyn, 30, 3);//20130124新增//20130226 10-->30 防止大缺口造成内外环断裂
		difference(regDyn,regSpecialPolar,&regDyn);
		fill_up(regDyn, &regDyn);
		opening_rectangle1 (regDyn, &regDyn, 3, 3);//开掉孤立细线

		connection(regDyn,&regDyn);
		select_shape(regDyn,&regDyn,"width","and",m_nWidth/5,99999);//避免口面太脏造成影响

		closing_rectangle1 (regDyn, &regClosing, 50, 2);//内/外环断开时防止分割错误
		difference(regClosing,regSpecialPolar,&regClosing);
		connection (regClosing, &regConn);
		select_shape (regConn, &regSel, "width", "and", m_nWidth*0.9, 9999);
		intersection (regSel, regDyn, &regDyn);
		fill_up (regDyn, &regDyn);
		union1 (regDyn, &regDyn);
		//2013.11.3 开掉边缘小干扰
		opening_rectangle1 (regDyn, &regDyn, 35, 1);
		closing_rectangle1 (regDyn, &regClosing,nDeformWidth, 1);
		difference(regDyn,regSpecialPolar,&regDyn);
		//2013.11.4 增加内外圈宽度限制
		connection (regClosing, &regConn);
		select_shape (regConn, &regConn, "height", "and", nDeformCirWid, 9999);
		union1 (regConn, &regClosing);

		difference (regClosing,regDyn, &regDifference);
		connection (regDifference,&regConn);
		opening_rectangle1 (regConn, &regConn, 1,2);
		union1 (regConn, &regConn);
		connection (regConn, &regConn);
		count_obj (regConn, &nNum);
		if (nNum<1)
		{
			return rtnInfo;
		}

		gen_empty_obj(&ErrorObjBorder);
		for (i =0;i<nNum;i++)
		{
			select_obj(regConn,&regSel,i+1);
			smallest_rectangle1 (regSel, &Row1, &Column1, &Row2, &Column2);
			intensity (regSel, imgPolar, &dGrayMean, NULL);
			//口变形高度和灰度
			if (Row2-Row1<nDeformHei || dGrayMean > nDeformGary)
			{
				continue;
			}
			concat_obj(regSel,ErrorObjBorder,&ErrorObjBorder);
		}

		//判断特殊干扰
		count_obj (ErrorObjBorder, &nNum); 
		//tupleUpOrDown 1:上>下;2:上<下;0：上=下
		//tupleKeystone 1:梯形;0：其它(包括矩形)
		tupleUpOrDown=HTuple();
		tupleKeystone=HTuple();
		for(i=1;i<nNum+1;i++) 
		{
			select_obj (ErrorObjBorder, &regSel, i);
			smallest_rectangle1 (regSel, &Row1, &Column1, &Row2, &Column2);
			get_region_points (regSel, &Rows, &Cols);
			tuple_length (Rows, &Length);
			int temp=0;
			int k=0;
			tupleRowPtNum=HTuple();
			tupleCloumnMin=HTuple();
			tupleCloumnMax=HTuple();
			int temprow = Rows[Hlong(Length[0])-1];
			//最后+1个，便于下面判断
			Rows.Append(temprow+1);				
			for(int j=0;j<Hlong(Length[0]);j++)
			{
				if(Rows[j]<Rows[j+1])
				{
					tupleRowPtNum.Append(j-temp+1);
					tupleCloumnMin.Append(Cols[temp]);
					tupleCloumnMax.Append(Cols[j]);
					temp=j+1;
					k=k+1;
				}
			}

			if(k>0)
			{
				int tempKeystone;
				if(tupleRowPtNum[0].I()>tupleRowPtNum[k-1].I())
				{
					tupleUpOrDown.Append(1);
					tempKeystone=1;
					//阶梯
					for(int j=0;j<k-1;j++) 
					{
						if(tupleCloumnMin[j]>tupleCloumnMin[j+1] || tupleCloumnMax[j]<tupleCloumnMax[j+1])
						{
							tempKeystone=0;
							break;
						}
					}
					//阶梯尺度
					if(tupleCloumnMin[k-1].I()-tupleCloumnMin[0].I()<=nDeformHei || tupleCloumnMax[0].I()-tupleCloumnMax[k-1].I()<=nDeformHei)
						tempKeystone=0;
					tupleKeystone.Append(tempKeystone);
				}
				else
				{	
					if(tupleRowPtNum[0].I()<tupleRowPtNum[k-1].I())
					{
						tupleUpOrDown.Append(2);
						tempKeystone=1;
						for(int j=0;j<k-1;j++) 
						{
							if(tupleCloumnMin[j]<tupleCloumnMin[j+1] || tupleCloumnMax[j]>tupleCloumnMax[j+1])
							{
								tempKeystone=0;
								break;
							}
						}
						if(tupleCloumnMin[0].I()-tupleCloumnMin[k-1].I()<=nDeformHei || tupleCloumnMax[k-1].I()-tupleCloumnMax[0].I()<=nDeformHei)
							tempKeystone=0;
						tupleKeystone.Append(tempKeystone);
					}
					else
					{
						tupleUpOrDown.Append(0);
						tupleKeystone.Append(0);
					}				
				}
			}
		}

		gen_empty_obj(&ErrorObj);	
		HTuple Newtuple= HTuple();
		tuple_gen_const (nNum, 1, &Newtuple);
		//干扰标志量
		int interfere;
		for(i =1;i<nNum+1;i++) 
		{
			select_obj (ErrorObjBorder, &regSel, i);
			int temp = Newtuple[i-1];
			if(temp==1)
			{
				Newtuple[i-1] = 0;
				area_center (regSel, NULL, &CenRow1, &CenCol1);
				interfere = 0;
				for(int j =1;j<nNum+1;j++)
				{
					if(j==i)
					{
						continue;
					}
					select_obj (ErrorObjBorder, &regSel1, j);
					area_center (regSel1, NULL, &CenRow2, &CenCol2);

					if(abs(CenRow2-CenRow1)<nDeformHei*2 && abs(CenCol2-CenCol1)<nDeformWidth*2 && tupleUpOrDown[i-1]==tupleUpOrDown[j-1])
					{
						interfere = 1;
						Newtuple[j-1] = 0;
					}
				}
				if(interfere==1)
				{
					continue;
				}
				int temp = tupleKeystone[i-1];
				if(temp==1)
				{
					concat_obj (regSel, ErrorObj, &ErrorObj);
				}
			}
		}

		union1(ErrorObj,&ErrorObj);
		connection(ErrorObj,&ErrorObj);

		count_obj(ErrorObj,&nNum);
		if (nNum>0)
		{	
			polar_trans_region_inv(ErrorObj,&ErrorObj,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
			concat_obj(rtnInfo.regError, ErrorObj, &rtnInfo.regError);
			rtnInfo.nType = ERROR_MOUTHDEFORM;
			return rtnInfo;

		}	
	}
	return rtnInfo;
}

void CCheck::calculate_lines_gauss_parameters (HTuple hv_MaxLineWidth, HTuple hv_Contrast, 
	HTuple *hv_Sigma, HTuple *hv_Low, HTuple *hv_High)
{
	HTuple  hv_ContrastHigh, hv_ContrastLow, hv_HalfWidth;
	HTuple  hv_Help;

	//Check control parameters
	HTuple test;
	tuple_length(hv_MaxLineWidth, &test);
	if (0 != (test!=1))
	{
		throw HException("Wrong number of values of control parameter: 1");
	}
	tuple_is_number(hv_MaxLineWidth, &test);
	if (0 != (test!=1))
	{
		throw HException("Wrong type of control parameter: 1");
	}
	if (0 != (hv_MaxLineWidth<=0))
	{
		throw HException("Wrong value of control parameter: 1");
	}

	tuple_length(hv_Contrast, &test);
	if (0 != ((test!=1) && (test!=2)) )
	{
		throw HException("Wrong number of values of control parameter: 2");
	}
	tuple_is_number(hv_Contrast, &test);
	tuple_min(test, &test);
	if (0 != (test==0))
	{
		throw HException("Wrong type of control parameter: 2");
	}
	//Set and check ContrastHigh
	hv_ContrastHigh = ((const HTuple&)hv_Contrast)[0];
	if (0 != (hv_ContrastHigh<0))
	{
		throw HException("Wrong value of control parameter: 2");
	}
	//Set or derive ContrastLow
	tuple_length(hv_Contrast, &test);
	if (0 != (test==2))
	{
		hv_ContrastLow = ((const HTuple&)hv_Contrast)[1];
	}
	else
	{
		hv_ContrastLow = hv_ContrastHigh/3.0;
	}
	//Check ContrastLow
	if (0 != (hv_ContrastLow<0))
	{
		throw HException("Wrong value of control parameter: 2");
	}
	if (0 != (hv_ContrastLow>hv_ContrastHigh))
	{
		throw HException("Wrong value of control parameter: 2");
	}
	//
	//Calculate the parameters Sigma, Low, and High for lines_gauss
	tuple_sqrt(3, &test);
	if (0 != (hv_MaxLineWidth<test))
	{
		//Note that LineWidthMax < sqrt(3.0) would result in a Sigma < 0.5,
		//which does not make any sense, because the corresponding smoothing
		//filter mask would be of size 1x1.
		//To avoid this, LineWidthMax is restricted to values greater or equal
		//to sqrt(3.0) and the contrast values are adapted to reflect the fact
		//that lines that are thinner than sqrt(3.0) pixels have a lower contrast
		//in the smoothed image (compared to lines that are sqrt(3.0) pixels wide).
		hv_ContrastLow = (hv_ContrastLow*hv_MaxLineWidth)/test;
		hv_ContrastHigh = (hv_ContrastHigh*hv_MaxLineWidth)/test;
		hv_MaxLineWidth = test;
	}
	//Convert LineWidthMax and the given contrast values into the input parameters
	//Sigma, Low, and High required by lines_gauss
	hv_HalfWidth = hv_MaxLineWidth/2.0;
	(*hv_Sigma) = hv_HalfWidth/test;
	hv_Help = ((-2.0*hv_HalfWidth)/((HTuple(6.283185307178).Sqrt())*((*hv_Sigma).Pow(3.0))))*((-0.5*((hv_HalfWidth/(*hv_Sigma)).Pow(2.0))).Exp());
	HTuple a,b;
	tuple_fabs(hv_ContrastHigh*hv_Help,&a);
	tuple_fabs(hv_ContrastLow*hv_Help,&b);
	(*hv_High) = a;
	(*hv_Low) = b;
	return;
}


// 注释-2017.12已做修改
////*功能：环形光口平面区域---2017.3修改
//RtnInfo CCheck::fnFRLMiddleReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
//{
//	RtnInfo rtnInfo;
//	s_pFRLMidReg pFRLMidReg = para.value<s_pFRLMidReg>();
//	s_oFRLMidReg oFRLMidReg = shape.value<s_oFRLMidReg>();
//	if (!pFRLMidReg.bEnabled)
//	{
//		rtnInfo.nType = GOOD_BOTTLE;
//		gen_empty_obj(&rtnInfo.regError);
//		return rtnInfo;
//	}
//
//	double dInRadius,dOutRadius;
//	Hobject regCheck,regThresh,regBin,regConnect,regSelect,regExcludeTemp;
//	Hobject reThres1, regThres2;
//	Hobject imgReduce,imgMean;
//	Hlong nNum;
//	int i;
//	smallest_circle(oFRLMidReg.oInCircle,NULL,NULL,&dInRadius);
//	smallest_circle(oFRLMidReg.oOutCircle,NULL,NULL,&dOutRadius);
//	if (dOutRadius<dInRadius+5)
//	{
//		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
//		rtnInfo.nType = ERROR_FINISHMID;
//		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
//		return rtnInfo;		
//	}
//	difference(oFRLMidReg.oOutCircle, oFRLMidReg.oInCircle, &regCheck);	
//	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
//	select_shape(regCheck,&regCheck,"area","and",100,99999999);
//	count_obj(regCheck,&nNum);
//	if (nNum==0)
//	{
//		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
//		rtnInfo.nType = ERROR_INVALID_ROI;
//		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
//		return rtnInfo;		
//	}
//
//	mean_image(imgSrc,&imgMean,31,31);
//	reduce_domain(imgSrc, regCheck, &imgReduce);
//	reduce_domain(imgMean, regCheck, &imgMean);
//	// 使用第一重条件提取
//	dyn_threshold(imgReduce, imgMean, &regThresh, pFRLMidReg.nEdge, "light");
//	threshold(imgReduce, &regBin, pFRLMidReg.nGray, 255);
//	union2(regThresh, regBin, &regThresh);
//	if (pFRLMidReg.bOpen)
//	{
//		copy_obj(regThresh,&regExcludeTemp,1,-1);
//		opening_circle(regThresh, &regThresh, pFRLMidReg.fOpenSize);
//		ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
//	}
//	connection(regThresh, &regConnect);
//	select_shape(regConnect, &regSelect, HTuple("area").Concat("ra"), "or", 
//		HTuple(pFRLMidReg.nArea).Concat(pFRLMidReg.nLen/2), HTuple(99999999).Concat(99999));
//	ExtExcludeDefect(rtnInfo,regConnect,regSelect,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
//	copy_obj (regSelect, &reThres1, 1, -1);
//
//	// 使用第二重条件提取
//	if (pFRLMidReg.nEdge_2!=pFRLMidReg.nEdge || pFRLMidReg.nGray_2!=pFRLMidReg.nGray)
//	{
//		dyn_threshold(imgReduce, imgMean, &regThresh, pFRLMidReg.nEdge_2, "light");
//		threshold(imgReduce, &regBin, pFRLMidReg.nGray_2, 255);
//		union2(regThresh, regBin, &regThresh);
//		if (pFRLMidReg.bOpen)
//		{
//			copy_obj(regThresh,&regExcludeTemp,1,-1);
//			opening_circle(regThresh, &regThresh, pFRLMidReg.fOpenSize);
//			ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
//		}
//		connection(regBin, &regConnect);
//		select_shape(regConnect, &regSelect, HTuple("area").Concat("ra"), "or", 
//			HTuple(pFRLMidReg.nArea_2).Concat(pFRLMidReg.nLen_2/2), HTuple(99999999).Concat(99999));
//		ExtExcludeDefect(rtnInfo,regConnect,regSelect,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLMidReg.strName,true);		
//		copy_obj (regSelect, &regThres2, 1, -1);
//	}
//
//	union2(reThres1, regThres2, &regThresh);
//
//	Hobject objSpecial,regSpecialPolar,regTemp;
//	gen_empty_obj(&objSpecial);
//	gen_empty_obj(&regSpecialPolar);
//	//排除干扰
//	if (m_bDisturb)
//	{
//		for (i=0;i<m_vDistItemID.size();++i)
//		{
//			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
//			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
//			if (!pDistReg.bEnabled)
//			{
//				continue;
//			}
//			Hobject xldVal,regDisturb;
//			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
//			if(pDistReg.nShapeType == 0)
//			{
//				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
//			}
//			else
//			{
//				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
//			}
//			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
//			count_obj(xldVal,&nNum);
//			if (nNum==0)
//			{
//				continue;
//			}
//			gen_region_contour_xld(xldVal,&regDisturb,"filled");
//			//判断该干扰区域是否与检测区域有交集
//			intersection(regCheck,regDisturb,&regDisturb);
//			select_shape(regDisturb,&regDisturb,"area","and",1,999999);
//			count_obj(regDisturb,&nNum);
//			if (nNum==0)
//			{
//				continue;
//			}
//			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
//			Hobject regCopy;
//			if (m_bExtExcludeDefect)
//			{
//				copy_obj(regThresh,&regCopy,1,-1);
//			}
//			//根据检测区域类型排除干扰
//			switch(pDistReg.nRegType)
//			{
//			case 0://完全不检
//				difference(regThresh,regDisturb,&regThresh);
//				concat_obj(regDisturb,objSpecial,&objSpecial);
//				break;
//			case 1://典型缺陷				
//				intersection(regThresh,regDisturb,&regTemp);
//				difference(regThresh,regDisturb,&regThresh);
//				opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
//				concat_obj(regThresh,regTemp,&regThresh);
//				concat_obj(regDisturb,objSpecial,&objSpecial);
//				ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
//				break;
//			case 2://排除条纹
//				{
//					Hobject regStripe;
//					//2013.9.16 nanjc 增独立参数提取排除
//					if (pDistReg.bStripeSelf)
//					{
//						distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
//					}
//					else
//					{
//						intersection(regDisturb,regThresh,&regTemp);
//						distRegStripe(pDistReg,regTemp,&regStripe);					
//					}
//					difference(regThresh,regStripe,&regThresh);
//					concat_obj(regStripe,objSpecial,&objSpecial);
//					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
//				}
//				break;
//			default:
//				break;
//			}
//		}
//	}
//
//	connection(regThresh, &regConnect);
//	select_shape(regConnect, &regSelect, "area", "and", 1, 99999999);
//	count_obj(regSelect, &nNum);
//	if (nNum > 0)
//	{
//		concat_obj(rtnInfo.regError,regSelect,&rtnInfo.regError);
//		rtnInfo.nType = ERROR_FINISHMID;
//		return rtnInfo;
//	}
//
//	// 检测口麻 [5/25/2012]
//	if (pFRLMidReg.bPitting)
//	{
//		dyn_threshold(imgReduce, imgMean, &regThresh, pFRLMidReg.nPitEdge, "light");
//		connection(regThresh, &regConnect);
//		select_shape(regConnect, &regSelect, "area", "and", pFRLMidReg.nPitArea, 99999999);
//		ExtExcludeDefect(rtnInfo,regConnect,regSelect,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_PITTING,pFRLMidReg.strName,true);		
//		count_obj(regSelect, &nNum);
//		if (nNum >= pFRLMidReg.nPitNum)
//		{
//			concat_obj(rtnInfo.regError,regSelect,&rtnInfo.regError);
//			rtnInfo.nType = ERROR_PITTING;
//			return rtnInfo;
//		}
//	}
//
//	// 检测剪刀印 [5/23/2012] /*20121123删除旧方法，改用新方法*/
//	if (pFRLMidReg.bLOF)
//	{
//		Hobject imgPolar,imgPlMean,imgCentMean,imgPlReduce,imgReduce2;
//		Hobject regDyn,regConn,regSel,regUnion,rectDomain,regDiff,
//			regClose,regTemp,regBin,regBinTemp,regBinBorder,regBoder,boder1,boder2;
//		Hobject ErrorObj,ErrorObjBorder,ErrorObjFinal;
//		Hlong row1,col1,row2,col2,row11,row21;		
//		double minDist,minDist1,minDist2;
//		int nExpand = 50;//内圈、外圈向内外扩展的宽度 Q:50->10（口平面宽度较小时，50会影响检测）
//		double nDist = dOutRadius-dInRadius+2*nExpand;
//		dLOFHei = nDist;
//		double dOriRow = currentOri.Row;
//		double dOriCol = currentOri.Col;
//		int nLOFLen = pFRLMidReg.nLOFLen; //剪刀印长度
//		float fLOFRab = pFRLMidReg.fLOFRab;
//		int nLOFLen_In = pFRLMidReg.nLOFLen_In;  //靠里环剪刀印长度
//		float fLOFRab_In = pFRLMidReg.fLOFRab_In;
//		int nLOFLen_Out = pFRLMidReg.nLOFLen_Out;  //靠外环剪刀印长度
//		float fLOFRab_Out = pFRLMidReg.fLOFRab_Out;
//		int nDiaMin = pFRLMidReg.nLOFDiaMin; //剪刀印内接圆直径下限
//		int nDiaMax = pFRLMidReg.nLOFDiaMax; //剪刀印内接圆直径上限
//		int nAngleOffset = pFRLMidReg.nLOFAngleOffset; //两个剪刀印夹角
//
//		//极坐标系下分割口平面区域（20130328由全图极坐标变换改进为扩展后的口平面区域变换，防止内外环报剪刀印）
//		polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
//			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
//		polar_trans_region(objSpecial,&regSpecialPolar,dOriRow,dOriCol,0,2*PI,
//			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
//		mean_image(imgPolar,&imgPlMean,31,31);
//		dyn_threshold(imgPolar,imgPlMean,&regDyn,5,"light");
//		difference(regDyn,regSpecialPolar,&regDyn);
//		//20130508 防止口面灰尘太多，影响口平面区域提取
//		connection(regDyn,&regDyn);
//		select_shape(regDyn,&regDyn,"area","and",10,99999999);
//		union1(regDyn,&regDyn);
//		//
//		opening_rectangle1 (regDyn, &regDyn, 3, 1);//2013.12.30 开掉麻点的影响
//		closing_rectangle1(regDyn,&regDyn,30,1);//20130124新增//20130226 10-->30 防止大缺口造成内外环断裂//20130715 10-->1,天马
//		difference(regDyn,regSpecialPolar,&regDyn);
//		connection(regDyn,&regDyn);
//		select_shape(regDyn,&regDyn,HTuple("anisometry").Concat("phi"),"and",HTuple(5).Concat(-0.35),
//			HTuple(9999).Concat(0.35));//20130703选择长宽比>5，且±20°的横向区域,防止南充内口太浅断开
//		union1(regDyn,&regDyn);
//		closing_rectangle1(regDyn,&regDyn,m_nWidth/3,2);//将最大1/3图像宽度的缺口填补
//		difference(regDyn,regSpecialPolar,&regDyn);
//		//select_shape(regDyn,&regDyn,"width","and",m_nWidth/5,99999);//避免口面太脏造成影响
//		//closing_rectangle1(regDyn,&regDyn,50,2);//内/外环断开时防止分割错误
//		connection(regDyn,&regConn);
//		select_shape(regConn,&regSel,"width","and",m_nWidth*0.9,9999);//0.9天马
//		union1(regSel,&regUnion);
//		count_obj(regUnion,&nNum);
//		if (nNum==0)
//		{
//			return rtnInfo;
//		}
//		smallest_rectangle1(regUnion,&row1,&col1,&row2,&col2);
//		gen_rectangle1(&rectDomain,row1,col1,row2,col2);
//		reduce_domain(imgPolar,rectDomain,&imgPlReduce);
//		difference(rectDomain,regUnion,&regDiff);	//2013.9.18 nanjc 修改：regDyn->regUnion，中环略亮干扰，报边界异常
//		closing_rectangle1(regDiff,&regClose,150,1);//剪刀印最大可检宽度为150
//		difference(regClose,regSpecialPolar,&regClose);
//		opening_rectangle1(regClose,&regClose,150,1);//20130124内环太浅时有问题
//		connection(regClose,&regClose);	
//		//	select_shape_std(regClose,&regSel,"max_area",70);	//20121217
//		select_shape(regClose,&regSel,
//			HTuple("width").Concat("height").Concat("rectangularity").Concat("area_holes"),"and",
//			HTuple(m_nWidth*0.9).Concat(nDist*0.5).Concat(0.7).Concat(0),
//			HTuple(m_nWidth*1.1).Concat(nDist).Concat(1.0).Concat(10));
//		count_obj(regSel,&nNum);
//		if (nNum==0)
//		{			
//			select_shape_std(regClose,&regSel,"max_area",70);			
//		} else if(nNum > 1)
//		{
//			select_shape_std(regSel,&regSel,"max_area",70);//20121228
//		}
//		union1(regSel,&regSel);		
//		dilation_rectangle1(regSel,&regSel,1,3);//上下扩展3像素，检测更全一些
//		erosion_rectangle1(regSel,&regSel,1,5);
//		closing_rectangle1 (regSel,&regSel, 1, 20);//内外环中间弧形油污影响
//		fill_up(regSel,&regSel);
//		//生成上下边界，用于增强靠边剪刀印
//		boundary(regSel,&regBoder,"inner");
//		clip_region(regBoder,&regBoder,0,1,m_nHeight,m_nWidth-2);
//		connection(regBoder,&regBoder);
//		count_obj(regBoder,&nNum);
//		if (nNum!=2)
//		{
//			//2015.1.28肇庆通产去掉该项错误
//			//gen_rectangle1(&rtnInfo.regError,120,20,220,120);
//			//rtnInfo.nType = ERROR_FINISHMID;
//			//rtnInfo.strEx = QObject::tr("The inner ring or the outer ring too small to cause border error when detecting scissors.");
//			return rtnInfo;	
//		}
//		sort_region (regBoder, &regBoder, "first_point", "true", "row");
//		select_obj(regBoder,&boder1,1);
//		select_obj(regBoder,&boder2,2);
//		///计算口平面区域宽度
//		gen_rectangle1(&regTemp,row1,m_nWidth/2-5,row2,m_nWidth/2+5);
//		intersection(regTemp,regSel,&regTemp);
//		smallest_rectangle1(regTemp,&row11,NULL,&row21,NULL);
//		float fLength=row21-row11;
//		if (fLength<(dOutRadius-dInRadius)/2.f)
//		{
//			//gen_rectangle1(&rtnInfo.regError,120,20,220,120);
//			//rtnInfo.nType = ERROR_FINISHMID;
//			//rtnInfo.strEx = QObject::tr("The inner ring or the outer ring too small to cause width error when detecting scissors.");
//			//return rtnInfo;	
//		}
//		else
//		{
//			reduce_domain(imgPlReduce,regSel,&imgReduce2);
//			//检测剪刀印
//			mean_image(imgReduce2,&imgCentMean,150,1);//横向强化平滑
//			dyn_threshold(imgReduce2,imgCentMean,&regDyn,pFRLMidReg.nLOFEdge,"light");
//			connection(regDyn,&regDyn);
//
//			select_shape(regDyn,&regDyn,"area","and",3,99999999);
//			union1(regDyn,&regDyn);
//			//2015.1.27 肇庆闭运算小于1Bug
//			closing_rectangle1(regDyn,&regDyn,1,max(1,fLength/10));//间隙小于1/10封盖面宽度的，被合并
//			difference(regDyn,regSpecialPolar,&regDyn);
//			opening_rectangle1(regDyn,&regDyn,1,2);//20130124内环有可能存在横向连接
//			connection(regDyn,&regBin);
//			select_shape(regBin,&regBin,"area","and",1,99999);//20130624防止面积为0的空区域造成异常
//			count_obj(regBin,&nNum);
//			if (nNum==0)
//			{
//				return rtnInfo;
//			}
//
//			//1.从中选出靠边剪刀印，用更小的长度单独检测
//			//2017.3增加---靠内环和靠外环的剪刀印提取标准不同
//			int minLOFLen2;
//			minLOFLen2 = min(nLOFLen_In, nLOFLen_Out);
//			select_shape(regBin, &regBinTemp, "height", "and", fLength*minLOFLen2/100, 99999);
//			count_obj(regBinTemp, &nNum);
//			if (nNum==0)
//			{
//				return rtnInfo;
//			}
//
//			gen_empty_obj(&ErrorObjBorder);		
//			for(i=1;i<=nNum;i++)
//			{
//				select_obj(regBinTemp, &regSel, i);
//				distance_rr_min(regSel, boder1, &minDist1, NULL, NULL, NULL, NULL);
//				distance_rr_min(regSel, boder2, &minDist2, NULL, NULL, NULL, NULL);
//                minDist = min(minDist1,minDist2);
//		        if (minDist<2) //靠近边界的距离小于2时，认为是边界剪刀印
//				{
//			        smallest_rectangle1(regSel, &row1, &col1, &row2, &col2);
//					col2=col2==col1?col2+1:col2;//防止极细的剪刀印宽度为0
//					float mouthTLRab=(float)(row2-row1)/(float)(col2-col1);
//					float mouthTLHeight=(float)row2-row1;   
//					// 区分靠里剪刀印与靠外剪刀印
//			        if(minDist == minDist1) //靠里剪刀印 
//					{
//				        if (mouthTLRab>fLOFRab_In && mouthTLHeight>fLength*nLOFLen_In/100)
//						{
//					        concat_obj(regSel, ErrorObjBorder, &ErrorObjBorder);
//				        }
//					}
//					else //靠外剪刀印
//					{
//				        if (mouthTLRab>fLOFRab_Out && mouthTLHeight>fLength*nLOFLen_Out/100)
//						{
//							concat_obj(regSel, ErrorObjBorder, &ErrorObjBorder);
//						}
//
//					}
//				}
//			}
//
//			//2. 检测整体的剪刀印
//			select_shape(regBin,&regBin,"height","and",fLength*nLOFLen/100,99999);
//			//此处不提取排除缺陷，否则由于剪刀印对比度要求较低，提取过多干扰
//			//copy_obj(regBin,&regExcludeTemp,1,-1);
//			//select_shape(regBin,&regBin,"height","and",fLength*nLOFLen/100,99999);
//			//ExtExcludePolarDefect(rtnInfo,regExcludeTemp,regBin,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_LOF,pFRLMidReg.strName,true);		
//
//			gen_empty_obj(&ErrorObj);		
//			count_obj(regBin,&nNum);
//			for (i=1;i<nNum+1;i++)
//			{			
//				select_obj(regBin,&regSel,i);			
//				smallest_rectangle1(regSel,&row1,&col1,&row2,&col2);	
//				col2=col2==col1?col2+1:col2;
//				float mouthTLRab=(float)(row2-row1)/(float)(col2-col1);
//				if (mouthTLRab>fLOFRab)
//				{						
//					concat_obj(regSel,ErrorObj,&ErrorObj);
//				}
//			}	
//			ExtExcludePolarDefect(rtnInfo,regBin,ErrorObj,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_LOF,pFRLMidReg.strName,true);				
//			concat_obj(ErrorObj,ErrorObjBorder,&ErrorObj);
//			union1(ErrorObj,&ErrorObj);
//			connection(ErrorObj,&ErrorObj);
//
//			count_obj(ErrorObj,&nNum);
//			if (nNum>0)
//			{	
//				polar_trans_region_inv(ErrorObj,&ErrorObj,dOriRow,dOriCol,0,2*PI,
//					dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
//
//				// 2017.3.20添加-宽度判断，防止其他白色缺陷干扰
//				nDiaMin = nDiaMin<nDiaMax?nDiaMin:nDiaMax;
//				select_shape (ErrorObj, &ErrorObjFinal, "inner_radius", "and", nDiaMin/2, nDiaMax/2);
//				ExtExcludeDefect(rtnInfo,ErrorObj,ErrorObjFinal,EXCLUDE_CAUSE_INRADIUS,EXCLUDE_TYPE_LOF,pFRLMidReg.strName);
//				count_obj (ErrorObjFinal, &nNum);
//				if(nNum > 0)
//				{
//					 if (nNum == 2) //2017.3添加-判断两条跨口线间的夹角，如果为180度，认为是模缝线非缺陷
//					 {
//						 HTuple ErrRow,ErrCol,Angle,Degree;
//						 double degree;
//						 area_center (ErrorObjFinal, NULL, &ErrRow, &ErrCol);
//						 angle_ll (dOriRow,dOriCol,ErrRow[0],ErrCol[0],dOriRow,dOriCol,ErrRow[1],ErrCol[1],&Angle);
//						 Degree = Angle.Deg();
//			             degree = fabs(Degree[0].D());
//					     if (fabs(degree-180) < nAngleOffset)
//						 {
//							 //模缝线
//					     }
//					 }  
//					 else
//					 {
//						 concat_obj(rtnInfo.regError, ErrorObjFinal, &rtnInfo.regError);
//						 rtnInfo.nType = ERROR_LOF;
//						 return rtnInfo;
//					 }
//				}	
//			}	
//		}		
//	}
//
//	//2013.11.2 索坤口变形,从C++中移植过来 nDeformWidth 改动 100--》150
//	//检测瓶口变形 [7/11/2013 Nanjc] 
//	//定义错误类型
//	if(pFRLMidReg.bDeform)
//	{
//		Hobject imgPolar,imgPlMean,imgCentMean,imgPlReduce,imgReduce2;
//		Hobject regDyn,regConn,regSel,regSel1,regClosing,regDifference;
//		Hobject ErrorObj,ErrorObjBorder;
//		Hlong Row1,Column1,Row2,Column2;
//		double CenRow1,CenCol1,CenRow2,CenCol2;
//		HTuple Rows, Cols,Length;
//		HTuple tupleRowPtNum,tupleCloumnMin,tupleCloumnMax,tupleUpOrDown,tupleKeystone;
//		double dGrayMean;
//		int nExpand = 35;//内圈、外圈向内外扩展的宽度*加宽，为将两个边缘都扩进来
//		double nDist = dOutRadius-dInRadius+2*nExpand;
//
//		double dOriRow = currentOri.Row;
//		double dOriCol = currentOri.Col;
//		int nDeformGary = pFRLMidReg.nDeformGary;
//		int nDeformHei = pFRLMidReg.nDeformHei;
//		int nDeformCirWid = pFRLMidReg.nDeformCirWid;
//
//		int nDeformWidth = 150;
//		//极坐标系下分割口平面区域（20130328由全图极坐标变换改进为扩展后的口平面区域变换，防止内外环报剪刀印）
//
//		polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
//			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
//		polar_trans_region(objSpecial,&regSpecialPolar,dOriRow,dOriCol,0,2*PI,
//			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
//		mean_image(imgPolar,&imgPlMean,31,31);
//		dyn_threshold(imgPolar,imgPlMean,&regDyn,5,"light");		
//		difference(regDyn,regSpecialPolar,&regDyn);
//
//		opening_rectangle1 (regDyn, &regDyn, 3, 1);//开掉麻点的影响
//		closing_rectangle1 (regDyn, &regDyn, 30, 3);//20130124新增//20130226 10-->30 防止大缺口造成内外环断裂
//		difference(regDyn,regSpecialPolar,&regDyn);
//		fill_up(regDyn, &regDyn);
//		opening_rectangle1 (regDyn, &regDyn, 3, 3);//开掉孤立细线
//
//		connection(regDyn,&regDyn);
//		select_shape(regDyn,&regDyn,"width","and",m_nWidth/5,99999);//避免口面太脏造成影响
//
//		closing_rectangle1 (regDyn, &regClosing, 50, 2);//内/外环断开时防止分割错误
//		difference(regClosing,regSpecialPolar,&regClosing);
//		connection (regClosing, &regConn);
//		select_shape (regConn, &regSel, "width", "and", m_nWidth*0.9, 9999);
//		intersection (regSel, regDyn, &regDyn);
//		fill_up (regDyn, &regDyn);
//		union1 (regDyn, &regDyn);
//		//2013.11.3 开掉边缘小干扰
//		opening_rectangle1 (regDyn, &regDyn, 35, 1);
//		closing_rectangle1 (regDyn, &regClosing,nDeformWidth, 1);
//		difference(regDyn,regSpecialPolar,&regDyn);
//		//2013.11.4 增加内外圈宽度限制
//		connection (regClosing, &regConn);
//		select_shape (regConn, &regConn, "height", "and", nDeformCirWid, 9999);
//		union1 (regConn, &regClosing);
//
//		difference (regClosing,regDyn, &regDifference);
//		connection (regDifference,&regConn);
//		opening_rectangle1 (regConn, &regConn, 1,2);
//		union1 (regConn, &regConn);
//		connection (regConn, &regConn);
//		count_obj (regConn, &nNum);
//		if (nNum<1)
//		{
//			return rtnInfo;
//		}
//
//		gen_empty_obj(&ErrorObjBorder);
//		for (i =0;i<nNum;i++)
//		{
//			select_obj(regConn,&regSel,i+1);
//			smallest_rectangle1 (regSel, &Row1, &Column1, &Row2, &Column2);
//			intensity (regSel, imgPolar, &dGrayMean, NULL);
//			//口变形高度和灰度
//			if (Row2-Row1<nDeformHei || dGrayMean > nDeformGary)
//			{
//				continue;
//			}
//			concat_obj(regSel,ErrorObjBorder,&ErrorObjBorder);
//		}
//
//		//判断特殊干扰
//		count_obj (ErrorObjBorder, &nNum); 
//		//tupleUpOrDown 1:上>下;2:上<下;0：上=下
//		//tupleKeystone 1:梯形;0：其它(包括矩形)
//		tupleUpOrDown=HTuple();
//		tupleKeystone=HTuple();
//		for(i=1;i<nNum+1;i++) 
//		{
//			select_obj (ErrorObjBorder, &regSel, i);
//			smallest_rectangle1 (regSel, &Row1, &Column1, &Row2, &Column2);
//			get_region_points (regSel, &Rows, &Cols);
//			tuple_length (Rows, &Length);
//			int temp=0;
//			int k=0;
//			tupleRowPtNum=HTuple();
//			tupleCloumnMin=HTuple();
//			tupleCloumnMax=HTuple();
//			int temprow = Rows[long(Length[0])-1];
//			//最后+1个，便于下面判断
//			Rows.Append(temprow+1);				
//			for(int j=0;j<long(Length[0]);j++)
//			{
//				if(Rows[j]<Rows[j+1])
//				{
//					tupleRowPtNum.Append(j-temp+1);
//					tupleCloumnMin.Append(Cols[temp]);
//					tupleCloumnMax.Append(Cols[j]);
//					temp=j+1;
//					k=k+1;
//				}
//			}
//
//			if(k>0)
//			{
//				int tempKeystone;
//				if(tupleRowPtNum[0].I()>tupleRowPtNum[k-1].I())
//				{
//					tupleUpOrDown.Append(1);
//					tempKeystone=1;
//					//阶梯
//					for(int j=0;j<k-1;j++) 
//					{
//						if(tupleCloumnMin[j]>tupleCloumnMin[j+1] || tupleCloumnMax[j]<tupleCloumnMax[j+1])
//						{
//							tempKeystone=0;
//							break;
//						}
//					}
//					//阶梯尺度
//					if(tupleCloumnMin[k-1].I()-tupleCloumnMin[0].I()<=nDeformHei || tupleCloumnMax[0].I()-tupleCloumnMax[k-1].I()<=nDeformHei)
//						tempKeystone=0;
//					tupleKeystone.Append(tempKeystone);
//				}
//				else
//				{	
//					if(tupleRowPtNum[0].I()<tupleRowPtNum[k-1].I())
//					{
//						tupleUpOrDown.Append(2);
//						tempKeystone=1;
//						for(int j=0;j<k-1;j++) 
//						{
//							if(tupleCloumnMin[j]<tupleCloumnMin[j+1] || tupleCloumnMax[j]>tupleCloumnMax[j+1])
//							{
//								tempKeystone=0;
//								break;
//							}
//						}
//						if(tupleCloumnMin[0].I()-tupleCloumnMin[k-1].I()<=nDeformHei || tupleCloumnMax[k-1].I()-tupleCloumnMax[0].I()<=nDeformHei)
//							tempKeystone=0;
//						tupleKeystone.Append(tempKeystone);
//					}
//					else
//					{
//						tupleUpOrDown.Append(0);
//						tupleKeystone.Append(0);
//					}				
//				}
//			}
//		}
//
//		gen_empty_obj(&ErrorObj);	
//		HTuple Newtuple= HTuple();
//		tuple_gen_const (nNum, 1, &Newtuple);
//		//干扰标志量
//		int interfere;
//		for(i =1;i<nNum+1;i++) 
//		{
//			select_obj (ErrorObjBorder, &regSel, i);
//			int temp = Newtuple[i-1];
//			if(temp==1)
//			{
//				Newtuple[i-1] = 0;
//				area_center (regSel, NULL, &CenRow1, &CenCol1);
//				interfere = 0;
//				for(int j =1;j<nNum+1;j++)
//				{
//					if(j==i)
//					{
//						continue;
//					}
//					select_obj (ErrorObjBorder, &regSel1, j);
//					area_center (regSel1, NULL, &CenRow2, &CenCol2);
//
//					if(abs(CenRow2-CenRow1)<nDeformHei*2 && abs(CenCol2-CenCol1)<nDeformWidth*2 && tupleUpOrDown[i-1]==tupleUpOrDown[j-1])
//					{
//						interfere = 1;
//						Newtuple[j-1] = 0;
//					}
//				}
//				if(interfere==1)
//				{
//					continue;
//				}
//				int temp = tupleKeystone[i-1];
//				if(temp==1)
//				{
//					concat_obj (regSel, ErrorObj, &ErrorObj);
//				}
//			}
//		}
//
//		union1(ErrorObj,&ErrorObj);
//		connection(ErrorObj,&ErrorObj);
//
//		count_obj(ErrorObj,&nNum);
//		if (nNum>0)
//		{	
//			polar_trans_region_inv(ErrorObj,&ErrorObj,dOriRow,dOriCol,0,2*PI,
//				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
//			concat_obj(rtnInfo.regError, ErrorObj, &rtnInfo.regError);
//			rtnInfo.nType = ERROR_MOUTHDEFORM;
//			return rtnInfo;
//
//		}	
//	}
//	return rtnInfo;
//}


//*功能：环形光外环区域
RtnInfo CCheck::fnFRLOuterReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{	
	RtnInfo rtnInfo;
	s_pFRLOutReg pFRLOutReg = para.value<s_pFRLOutReg>();
	s_oFRLOutReg oFRLOutReg = shape.value<s_oFRLOutReg>();
	if (!pFRLOutReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject regCheck,regThresh,regBin,regDynThresh,regClose,regOpen,regSelected,regBorder,regBorderSkeleton,regDiff,regExcludeTemp;
	Hobject imgReduce,imgMean;
	double dInRadius,dOutRadius;
	Hlong nNum;
	smallest_circle(oFRLOutReg.oInCircle,NULL,NULL,&dInRadius);
	smallest_circle(oFRLOutReg.oOutCircle,NULL,NULL,&dOutRadius);
	if (dOutRadius<dInRadius+5)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_FINISHOUT;
		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
		return rtnInfo;		
	}
	difference(oFRLOutReg.oOutCircle,oFRLOutReg.oInCircle,&regCheck);	
	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regCheck,&regCheck,"area","and",100,99999999);
	count_obj(regCheck,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	//2015.1.4 方圆需求，口内外环，增加对比度
	//检测外环缺陷
	//difference(oFRLOutReg.oOutCircle,oFRLOutReg.oInCircle,&regCheck);	
	//reduce_domain(imgSrc,regCheck,&imgReduce);
	//threshold(imgReduce,&regThresh,pFRLOutReg.nGray,255);
	mean_image(imgSrc,&imgMean,31,31);
	reduce_domain(imgSrc, regCheck, &imgReduce);
	reduce_domain(imgMean,regCheck,&imgMean);
	dyn_threshold(imgReduce, imgMean, &regThresh, pFRLOutReg.nEdge, "light");
	threshold(imgReduce, &regBin, pFRLOutReg.nGray, 255);
	union2(regThresh, regBin, &regThresh);

	int i;
	Hobject objSpecial,regSpecialPolar,regTemp;
	gen_empty_obj(&objSpecial);
	gen_empty_obj(&regSpecialPolar);
	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(regCheck,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regThresh,regDisturb,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
				case 1://典型缺陷				
					intersection(regThresh,regDisturb,&regTemp);
					difference(regThresh,regDisturb,&regThresh);
					opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
					concat_obj(regThresh,regTemp,&regThresh);
					concat_obj(regDisturb,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					break;
				case 2://排除条纹
					{
						Hobject regStripe;
						//2013.9.16 nanjc 增独立参数提取排除
						if (pDistReg.bStripeSelf)
						{
							distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
						}
						else
						{
							intersection(regDisturb,regThresh,&regTemp);
							distRegStripe(pDistReg,regTemp,&regStripe);					
						}
						difference(regThresh,regStripe,&regThresh);
						concat_obj(regStripe,objSpecial,&objSpecial);
						ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					}
					break;
			default:
				break;
			}
		}
	}
	closing_circle(regThresh, &regClose, 3.5);
	opening_circle(regClose,&regOpen,pFRLOutReg.fOpenSize);
	//ExtExcludeDefect(rtnInfo,regClose,regOpen,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLOutReg.strName,true);
	difference(regOpen,objSpecial,&regOpen);
	connection(regOpen, &regOpen);
	select_shape(regOpen, &regSelected, "area","and",pFRLOutReg.nArea,99999999);
	ExtExcludeDefect(rtnInfo,regOpen,regSelected,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pFRLOutReg.strName,true);

	count_obj(regSelected, &nNum);
	if (nNum > 0)
	{
		concat_obj(rtnInfo.regError,regSelected,&rtnInfo.regError);
		rtnInfo.nType = ERROR_FINISHOUT;
		return rtnInfo;
	}

	//检测大破口--断环
	if (pFRLOutReg.bBrokenRing)
	{
		mean_image(imgSrc,&imgMean,31,31);
		reduce_domain(imgMean,regCheck,&imgMean);

		dyn_threshold(imgReduce,imgMean,&regDynThresh,pFRLOutReg.nBrokenRingEdge,"light");
		threshold(imgReduce,&regThresh,pFRLOutReg.nBrokenRingGray,255);
		union2(regDynThresh,regThresh,&regThresh);
		//2014.1.7nanjc 灌装线检外环断环+开运算
		//		opening_circle(regThresh,&regThresh,pFRLOutReg.fBrokenOpenSize);
		closing_circle(regThresh,&regThresh,3.5);
		//		difference(regThresh,objSpecial,&regThresh);
		connection(regThresh,&regThresh);
		select_shape(regThresh, &regBorder, "contlength", "and", dOutRadius, 99999999);

		count_obj(regBorder, &nNum);
		if (nNum > 0)
		{
			union1(regBorder, &regBorder);
			//closing_circle(regBorder, &regBorder, nBreachLen/2);20120625取消tyx
			connection(regBorder, &regBorder);

			select_shape(regBorder, &regBorder, "contlength", "and", dOutRadius, 99999999);
			count_obj(regBorder, &nNum);
			if (nNum > 0)
			{
				select_shape_std(regBorder, &regBorder, "max_area", 70);
				fill_up(regBorder, &regBorder);
				double dCircu;
				Hobject BorderSkeleton, RegDiff;
				circularity(regBorder, &dCircu);
				if (dCircu < 0.2)
				{
					//2014.9.19 增加开运算，开掉毛刺
					opening_circle(regBorder,&regBorder,pFRLOutReg.fBrokenOpenSize);
					skeleton(regBorder, &regBorderSkeleton);
					shape_trans(regBorderSkeleton, &regBorderSkeleton, "convex");
					boundary(regBorderSkeleton, &regBorderSkeleton, "inner");
					difference(regBorderSkeleton, regBorder, &regDiff);
					connection(regDiff, &regExcludeTemp);
					select_shape(regExcludeTemp, &regDiff, "contlength", "and", 2*pFRLOutReg.nBrokenRingLen, 99999999);
					ExtExcludeDefect(rtnInfo,regExcludeTemp,regDiff,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_BROKENRING,pFRLOutReg.strName,true);
					count_obj(regDiff, &nNum);
					if (nNum > 0)
					{
						concat_obj(rtnInfo.regError,regDiff,&rtnInfo.regError);
						rtnInfo.nType = ERROR_BROKENRING;						
						return rtnInfo;	
					}
				}
			}
		}
	}

	//2013.12.5 灌装线检瓶口外环小缺口，跟大缺口放到一起//该缺陷叫外环缺口，之前的外环缺口叫外环断环
	if (pFRLOutReg.bBreach)
	{
		//2013.11.2 索坤口变形,从C++中移植过来 nDeformWidth 改动 100--》150
		//检测瓶口变形 [7/11/2013 Nanjc] 
		//定义错误类型
		Hobject imgPolar,imgPlMean,imgCentMean,imgPlReduce,imgReduce2;
		Hobject regDyn,regOuter,regConn,regSel,regSel1,regClosing,regDifference,regTempOpen;
		Hobject ErrorObj,ErrorObjBorder;
		Hobject regRect1,regRect2;
		Hlong row1,col1,row2,col2;	
		//HTuple Rows1, Cols1,Rows2, Cols2;
		HTuple tupleRowPtNum,tupleCloumnMin,tupleCloumnMax,tupleUpOrDown,tupleKeystone;
		int nExpand = 5;//内圈、外圈向内外扩展的宽度*加宽，为将两个边缘都扩进来：外环此处35->5
		double nDist = dOutRadius-dInRadius+2*nExpand;
		int i;
		//int iPolarImageWidth = (dOutRadius+dInRadius)*PI;
		//int iPolarImageHeight = nDist;

		double dOriRow = currentOri.Row;
		double dOriCol = currentOri.Col;

		int nBreachWidthMax = 200;

		//极坐标系
		polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		polar_trans_image_ext(objSpecial,&regSpecialPolar,dOriRow,dOriCol,0,2*PI,
			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		mean_image(imgPolar,&imgPlMean,31,31);
		dyn_threshold(imgPolar,imgPlMean,&regDyn,pFRLOutReg.nBreachEdge,"light");		
		opening_rectangle1 (regDyn, &regDyn, 3, 1);//开掉麻点的影响
		closing_rectangle1 (regDyn, &regDyn, 10, 3);//20130124新增//20130226 10-->30 防止大缺口造成外环断裂
		fill_up(regDyn, &regDyn);
		opening_rectangle1 (regDyn, &regDyn, 3, 3);//开掉孤立细线

		//connection(regDyn,&regDyn);
		//select_shape(regDyn,&regDyn,"width","and",m_nWidth/5,99999);//避免口面太脏造成影响

		opening_rectangle1 (regDyn, &regDyn, 10, 1);//开掉干扰
		closing_rectangle1 (regDyn, &regClosing, 80, 1);//内/外环断开时防止分割错误
		difference(regClosing,regSpecialPolar,&regClosing);

		connection (regClosing, &regConn);
		select_shape (regConn, &regSel, "width", "and", m_nWidth*0.9, 9999);
		intersection (regSel, regDyn, &regDyn);

		fill_up (regDyn, &regDyn);
		union1 (regDyn, &regDyn);

		//2013.11.3 开掉边缘小干扰
		//opening_rectangle1 (regDyn, &regDyn, 30, 1);
		//2014.2.28 为防止缺口图像靠边缘时，小块正常凸起被开掉的现象，将靠边(左右)开掉区域+回去
		opening_rectangle1 (regDyn, &regTempOpen, 30, 1);
		difference(regDyn,regTempOpen,&regDifference);
		connection(regDifference,&regDifference);
		count_obj(regDifference, &nNum);
		for (i = 0;i<nNum;i++)
		{
			select_obj(regDifference,&regSel,i+1);
			smallest_rectangle1(regSel,NULL,&col1,NULL,&col2);
			if (col1<5 || col2 >m_nWidth-5)
			{
				concat_obj(regTempOpen,regSel,&regTempOpen);
			}
		}
		union1 (regTempOpen, &regDyn);
		//////////////////////////////////////////////////////////////////////////
		closing_rectangle1 (regDyn, &regClosing,nBreachWidthMax, 1);
		difference(regClosing,regSpecialPolar,&regClosing);

		difference (regClosing,regDyn, &regDifference);
		connection (regDifference,&regConn);

		opening_rectangle1 (regConn, &regConn, 1,2);
		union1 (regConn, &regConn);
		connection (regConn, &regConn);
		count_obj (regConn, &nNum);
		gen_empty_obj(&ErrorObj);
		for (i=1;i<nNum+1;i++)
		{			
			select_obj(regConn,&regSel,i);			
			smallest_rectangle1(regSel,&row1,&col1,&row2,&col2);				
			if (row2-row1>=pFRLOutReg.nBreachLen-2 && col2-col1>=pFRLOutReg.nBreachWidth)
			{						
				concat_obj(regSel,ErrorObj,&ErrorObj);
			}
		}
		ExtExcludePolarDefect(rtnInfo,regConn,ErrorObj,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_WIDTH_AND_HEIGHT,EXCLUDE_TYPE_BREACH,pFRLOutReg.strName,true);
		count_obj (ErrorObj, &nNum);
		if (nNum>0)
		{
			polar_trans_region_inv(ErrorObj,&ErrorObj,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
			concat_obj(rtnInfo.regError,ErrorObj,&rtnInfo.regError);
			rtnInfo.nType = ERROR_BREACH;	
			return rtnInfo;
		}
	}
	return rtnInfo;
}
//*功能：背光内环区域
RtnInfo CCheck::fnFBLInnerReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pFBLInReg pFBLInReg = para.value<s_pFBLInReg>();
	s_oFBLInReg oFBLInReg = shape.value<s_oFBLInReg>();
	if (!pFBLInReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hlong nNum;
	Hobject imgReduce,regCheck,regExcludeTemp;
	Hobject ImaAmp,ImaDir,RegLight,RegClosing,RegCon;
	Hobject RegSel,RegSelFinal,RegOpening;

	gen_region_contour_xld(oFBLInReg.oPolygon,&regCheck,"filled");
	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regCheck,&regCheck,"area","and",100,99999999);
	count_obj(regCheck,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}
	double radius;
	inner_circle (regCheck, NULL, NULL,&radius);
	if (radius > 2) //edges_image滤波模板至少为3x3
	{
		reduce_domain(imgSrc,regCheck,&imgReduce);
		edges_image (imgReduce, &ImaAmp, &ImaDir, "canny", 1, "nms", pFBLInReg.nLOFEdge, pFBLInReg.nLOFEdgeH); //20,30
		threshold (ImaAmp, &RegLight, 0, 255);
		closing_rectangle1 (RegLight, &RegClosing, 15, 1); //7->11
		connection (RegClosing, &RegCon);
		select_shape (RegCon, &RegSel, "area", "and", 5, 99999999);  //初步提取
		select_shape (RegSel, &RegSel, "height", "and", 5, 99999);
		opening_rectangle1 (RegSel, &RegOpening, 1, pFBLInReg.fLOFOpenSize);
		// 注释掉：提示的非缺陷太多
		//ExtExcludeDefect(rtnInfo,RegSel,RegOpening,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_LOF_BL,pFBLInReg.strName,true);
		connection (RegOpening, &RegCon);
		select_shape (RegCon, &RegSel, "area", "and", pFBLInReg.nLOFArea, 99999);
		ExtExcludeDefect(rtnInfo,RegCon,RegSel,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_LOF_BL,pFBLInReg.strName,true);
		select_shape(RegSel, &RegSelFinal,"ra","and",pFBLInReg.nLOFHeight/2,999999);
		ExtExcludeDefect(rtnInfo,RegSel,RegSelFinal,EXCLUDE_CAUSE_LENGTH,EXCLUDE_TYPE_LOF_BL,pFBLInReg.strName,true);
		if (pFBLInReg.nLOFPhiH >= pFBLInReg.nLOFPhiL)
		{
			select_shape(RegSelFinal, &RegSel, HTuple("phi").Concat("phi"), "or", HTuple(-0.0175*pFBLInReg.nLOFPhiH).Concat(0.0175*pFBLInReg.nLOFPhiL), HTuple(-0.0175*pFBLInReg.nLOFPhiL).Concat(0.0175*pFBLInReg.nLOFPhiH));
			ExtExcludeDefect(rtnInfo,RegSelFinal,RegSel,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_LOF_BL,pFBLInReg.strName,true);
		}
		select_gray(RegSel, imgReduce, &RegSelFinal, "mean", "and", pFBLInReg.nLOFMeanGray, 255);
		ExtExcludeDefect(rtnInfo,RegSel,RegSelFinal,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_LOF_BL,pFBLInReg.strName,true);
		count_obj (RegSelFinal, &nNum);
		if (nNum>0)
		{
			concat_obj(rtnInfo.regError,RegSelFinal,&rtnInfo.regError);
			rtnInfo.nType = ERROR_LOF_BL;
			return rtnInfo;
		}
	}
	return rtnInfo;

	// 2017.3---暂时注释，目前山东药玻还在用这种方式
	//RtnInfo rtnInfo;
	//s_pFBLInReg pFBLInReg = para.value<s_pFBLInReg>();
	//s_oFBLInReg oFBLInReg = shape.value<s_oFBLInReg>();
	//if (!pFBLInReg.bEnabled)
	//{
	//	rtnInfo.nType = GOOD_BOTTLE;
	//	gen_empty_obj(&rtnInfo.regError);
	//	return rtnInfo;
	//}

	//int zoomRadius = 3;//中心区域半径缩小值（像素），防止内口边缘误报
	//Hlong nNum;
	//double dInRadius,dOutRadius;
	//Hobject imgReduce,imgMean;
	//Hobject regCheck,regDyn,regExcludeTemp;
	//smallest_circle(oFBLInReg.oInCircle,NULL,NULL,&dInRadius);
	//smallest_circle(oFBLInReg.oOutCircle,NULL,NULL,&dOutRadius);
	//if (dOutRadius<dInRadius+5)
	//{
	//	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	//	rtnInfo.nType = ERROR_FINISHIN;
	//	rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
	//	return rtnInfo;		
	//}

	//difference(oFBLInReg.oOutCircle,oFBLInReg.oInCircle,&regCheck);
	//clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	//select_shape(regCheck,&regCheck,"area","and",100,999999);
	//count_obj(regCheck,&nNum);
	//if (nNum==0)
	//{
	//	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	//	rtnInfo.nType = ERROR_INVALID_ROI;
	//	rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
	//	return rtnInfo;		
	//}

	//reduce_domain(imgSrc,regCheck,&imgReduce);
	//mean_image(imgSrc,&imgMean,31,31);
	//reduce_domain(imgMean,regCheck,&imgMean);
	//dyn_threshold(imgReduce,imgMean,&regDyn,pFBLInReg.nEdge,"dark");
	//closing_circle(regDyn,&regExcludeTemp,3.5);
	//opening_circle(regExcludeTemp,&regDyn,pFBLInReg.fOpenSize);	
	//ExtExcludeDefect(rtnInfo,regExcludeTemp,regDyn,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pFBLInReg.strName,true);
	//connection(regDyn,&regExcludeTemp);
	//select_shape(regExcludeTemp,&regDyn,"area","and",pFBLInReg.nArea,99999);
	//ExtExcludeDefect(rtnInfo,regExcludeTemp,regDyn,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pFBLInReg.strName,true);
	//count_obj(regDyn,&nNum);
	//if (nNum>0)
	//{
	//	concat_obj(rtnInfo.regError,regDyn,&rtnInfo.regError);
	//	rtnInfo.nType = ERROR_FINISHIN;
	//	return rtnInfo;
	//}
	//return rtnInfo;
}
//*功能：背光口平面区域
RtnInfo CCheck::fnFBLMiddleReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pFBLMidReg pFBLMidReg = para.value<s_pFBLMidReg>();
	s_oFBLMidReg oFBLMidReg = shape.value<s_oFBLMidReg>();
	if (!pFBLMidReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hlong nNum;
	double dInRadius,dOutRadius,phi;
	double dOriRow = currentOri.Row;
	double dOriCol = currentOri.Col;
	float fShadowAng = pFBLMidReg.nShadowAng*(2.f*PI/360.f);	
	Hobject objError,regDyn,objSel,regExcludeTemp;
	Hobject imgPolar,imgMean;
	smallest_circle(oFBLMidReg.oInCircle,NULL,NULL,&dInRadius);
	smallest_circle(oFBLMidReg.oOutCircle,NULL,NULL,&dOutRadius);
	if (dOutRadius<dInRadius+5)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_FINISHMID;
		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
		return rtnInfo;		
	}

	gen_empty_obj(&objError);
	polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,dInRadius,dOutRadius,
		m_nWidth,dOutRadius-dInRadius,"nearest_neighbor");
	mean_image(imgPolar,&imgMean,150,1);
	dyn_threshold(imgPolar,imgMean,&regDyn,pFBLMidReg.nEdge,"dark");
	connection(regDyn,&regDyn);
	//select_shape(regDyn,&regDyn,HTuple("area").Concat("height"),"and",
	//	HTuple(pFBLMidReg.nArea).Concat(2),HTuple(999999).Concat(9999));
	select_shape(regDyn,&regExcludeTemp,HTuple("area"),"and",HTuple(pFBLMidReg.nArea),HTuple(99999999));
	ExtExcludePolarDefect(rtnInfo,regDyn,regExcludeTemp,dInRadius,dOutRadius,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_BACKMOUTHMID,pFBLMidReg.strName,true);
	select_shape(regExcludeTemp,&regDyn,HTuple("height"),"and",HTuple(2),HTuple(99999));
	ExtExcludePolarDefect(rtnInfo,regExcludeTemp,regDyn,dInRadius,dOutRadius,EXCLUDE_CAUSE_HEIGHT,EXCLUDE_TYPE_BACKMOUTHMID,pFBLMidReg.strName,false);
	//排除环形阴影
	if(pFBLMidReg.bShadow)
	{
		count_obj(regDyn,&nNum);
		for (int i=1; i<nNum+1;i++)
		{
			select_obj(regDyn,&objSel,i);
			orientation_region(objSel,&phi);
			if ((fabs(phi)<-fShadowAng+PI)&&(fabs(phi)>fShadowAng))
				concat_obj(objSel,objError,&objError);
		}
	}
	else
	{
		copy_obj(regDyn,&objError,1,-1);
	}
	count_obj(objError,&nNum);
	ExtExcludePolarDefect(rtnInfo,regDyn,objError,dInRadius,dOutRadius,EXCLUDE_CAUSE_SHADOW,EXCLUDE_TYPE_BACKMOUTHMID,pFBLMidReg.strName,false);
	if (nNum>0)
	{
		polar_trans_region_inv(objError,&objError,dOriRow,dOriCol,0,2*PI,dInRadius,dOutRadius,
			m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");
		concat_obj(rtnInfo.regError,objError,&rtnInfo.regError);
		rtnInfo.nType = ERROR_FINISHMID;
		return rtnInfo;
	}

	return rtnInfo;
}
//*功能：瓶底内环区域
//2017.4修改内容：一般缺陷和圆斑增加排除气泡功能
RtnInfo CCheck::fnBInnerReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pBInReg pBInReg = para.value<s_pBInReg>();
	s_oBInReg oBInReg = shape.value<s_oBInReg>();
	if (!pBInReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double dInRadius,dOutRadius,dRegRow,dRegCol,dMeanGray;
	Hobject regValid,regThresh1,regThresh2,regSelect,regGray,RegSel,regExcludeTemp,regDyn;
	Hobject imgReduce,imgMean;
	HTuple tpOperation1,tpOperation2;
	Hlong nNum,nTemp,nSubset, nAreaMin, nAreaMax;
	int i, j;
	Hobject objSpecial,regTemp;
	gen_empty_obj(&objSpecial);

	bool bModelMatch;
	bModelMatch = true; //2017.4-花纹匹配是否成功
	int debug = 1;
	// 计算有效检测区域 [3/27/2016 zhaodt]
	dInRadius = 0;
	dOutRadius = 0;
	smallest_circle(oBInReg.oOutCircle,&dRegRow,&dRegCol,&dOutRadius);
	dRegRow = currentOri.Row;
	dRegCol = currentOri.Col;
	if (dOutRadius<5)
	{
		dOutRadius+=5;
		gen_circle(&oBInReg.oOutCircle,dRegRow,dRegCol,dOutRadius);
		shape.setValue(oBInReg);
	}
	switch(pBInReg.nMethodIdx)
	{
	case 0:
		copy_obj(oBInReg.oOutCircle,&regValid,1,-1);
		break;
	case 1:
		smallest_circle(oBInReg.oInCircle,NULL,NULL,&dInRadius);
		smallest_circle(oBInReg.oOutCircle,NULL,NULL,&dOutRadius);
		if (dOutRadius<dInRadius+5)
		{
			gen_rectangle1(&rtnInfo.regError,120,20,220,120);
			rtnInfo.nType = ERROR_INVALID_ROI;
			rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
			return rtnInfo;		
		}
		difference(oBInReg.oOutCircle, oBInReg.oInCircle, &regValid);	
		break;
	case 2:
		gen_region_contour_xld(oBInReg.oRectBase,&regValid,"filled");
		//if(debug)
		//{
		//	write_contour_xld_dxf(oBInReg.oRectBase,".\\Locat9--UsedRect.dxf");
		//}
		break;
	case 3:
		gen_region_contour_xld(oBInReg.oTriBase,&regValid,"filled");
		break;
	default:
		gen_empty_obj(&regValid);
		break;
	}
	Hobject dstp;
	copy_obj(regValid,&dstp,1,-1);//inorder to display
	/*if(pBInReg.nMethodIdx == 2)
	{
		s_ROIPara roiPara;
		roiPara.fRoiRatio = 0.75;
		roiPara.nClosingWH = 20;
		roiPara.nGapWH = 100;
		rtnInfo = genValidROI(imgSrc,roiPara,regValid,&regValid);
		if (rtnInfo.nType>0)
		{
			return rtnInfo;
		}
	}*/
	clip_region(regValid,&regValid,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regValid,&regValid,"area","and",100,999999999);
	count_obj(regValid,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	//特殊缺陷-检测亮暗底[20120525]
	//if (pBInReg.bBottomDL)
	//{
	//	intensity(oBInReg.oOutCircle,imgSrc,&dMeanGray,NULL);
	//	if (dMeanGray<pBInReg.nDarkB || dMeanGray>pBInReg.nLightB)
	//	{
	//		concat_obj(rtnInfo.regError,oBInReg.oOutCircle,&rtnInfo.regError);
	//		rtnInfo.nType = ERROR_BOTTOM_DL;
	//		return rtnInfo;
	//	}
	//}
	//liuxu 20180910
	if (pBInReg.bBottomDL)
	{
		intensity(dstp,imgSrc,&dMeanGray,NULL);
		if (dMeanGray<pBInReg.nDarkB || dMeanGray>pBInReg.nLightB)
		{
			concat_obj(rtnInfo.regError,dstp,&rtnInfo.regError);
			rtnInfo.nType = ERROR_BOTTOM_DL;
			return rtnInfo;
		}
	}
	// 排除字符区域-排除花纹（模板匹配）---2017.4修改（顺序提前）
	if (pBInReg.bChar)
	{
		if (oBInReg.ModelID >= 0)
		{
			HTuple dRow1,dCol1,dAngle,dScore;
			double dPhi;
			Hobject objResult,objTemp;
			gen_empty_obj(&objResult);
			reduce_domain(imgSrc, regValid, &imgReduce);
			find_shape_model(imgReduce,oBInReg.ModelID,-PI,PI,0.25,1,0.2,"least_squares",0,0.9,&dRow1,&dCol1,&dAngle,&dScore); //0.1->0.25
			if (dScore.Num()>0)
			{
				line_orientation(dRow1,dCol1,dRegRow,dRegCol, &dPhi);
				if (dCol1 < dRegCol && dRow1 > dRegRow)
				{
					dPhi -= PI;
				}
				else if (dCol1 < dRegCol && dRow1 <= dRegRow)
				{
					dPhi += PI;
				}
				HTuple hommat2D;
				hom_mat2d_identity(&hommat2D);
				hom_mat2d_rotate(hommat2D, dPhi-pBInReg.dModelPhi, dRegRow, dRegCol, &hommat2D);
				affine_trans_contour_xld(oBInReg.oCharReg, &oBInReg.oCurCharReg, hommat2D);
				double dTemp;
				smallest_circle(oBInReg.oMarkReg, NULL, NULL, &dTemp);
				gen_circle(&oBInReg.oCurMarkReg, dRow1, dCol1, dTemp);
				shape.setValue(oBInReg);

				gen_region_contour_xld(oBInReg.oCurCharReg, &objTemp, "filled");
				concat_obj(oBInReg.oCurMarkReg, objTemp, &objResult);
				dilation_circle(objResult, &objResult, 7.5);
				union1(objResult, &objResult);
				difference(regValid, objResult, &regValid);
				concat_obj(objSpecial, objResult, &objSpecial);
			}
			else //如果没有匹配上
			{
				bModelMatch = false;
			}
		}
	}

	gen_empty_obj(&regThresh1);
	gen_empty_obj(&regThresh2);
	gen_empty_obj(&regDyn);
	reduce_domain(imgSrc, regValid, &imgReduce);
	mean_image(imgReduce, &imgMean, 51, 51);
	// 使用第一组条件分割
	dyn_threshold(imgReduce, imgMean, &regThresh1, pBInReg.nEdge1, "dark");
	connection(regThresh1, &regExcludeTemp);
	concat_obj(regDyn, regExcludeTemp, &regDyn);
	if (pBInReg.nOperation1==0)
	{
		tpOperation1 = "or";
	}
	else if (pBInReg.nOperation1==1)
	{
		tpOperation1 = "and";
	}
	select_shape(regExcludeTemp, &regThresh1, HTuple("area").Concat("ra"), tpOperation1, 
		HTuple(pBInReg.nArea1).Concat(pBInReg.nLen1/2), HTuple(99999999).Concat(99999));	
	ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh1,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pBInReg.strName,true);
	count_obj(regThresh1,&nNum);
	if (nNum>0)
	{
		select_gray(regThresh1, imgReduce, &regGray, "mean", "and", 0, pBInReg.nMeanGray1);
		ExtExcludeDefect(rtnInfo,regThresh1,regGray,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pBInReg.strName,true);
		copy_obj(regGray,&regThresh1,1,-1);/**/
	}
	// 使用第二组条件分割
	if (pBInReg.nEdge2 != pBInReg.nEdge1 || pBInReg.nMeanGray1 != pBInReg.nMeanGray2
		/*|| pBInReg.nArea1 != pBInReg.nArea2 || pBInReg.nLen1 != pBInReg.nLen2*/)
	{		
		dyn_threshold(imgReduce, imgMean, &regThresh2, pBInReg.nEdge2, "dark");
		connection(regThresh2, &regExcludeTemp);
		concat_obj(regDyn, regExcludeTemp, &regDyn);
		if (pBInReg.nOperation2==0)
		{
			tpOperation2 = "or";
		}
		else if (pBInReg.nOperation2==1)
		{
			tpOperation2 = "and";
		}
		select_shape(regExcludeTemp, &regThresh2, HTuple("area").Concat("ra"), tpOperation2, 
			HTuple(pBInReg.nArea2).Concat(pBInReg.nLen2/2), HTuple(99999999).Concat(99999));		
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh2,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pBInReg.strName,true);
		count_obj(regThresh2,&nNum);
		if (nNum>0)
		{
			select_gray(regThresh2, imgReduce, &regGray, "mean", "and", 0, pBInReg.nMeanGray2);
			ExtExcludeDefect(rtnInfo,regThresh2,regGray,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pBInReg.strName,true);
			copy_obj(regGray,&regThresh2,1,-1);/**/
		}
	}

	// 合并两种条件的计算结果 [3/28/2016 TT]
	concat_obj(regThresh1, regThresh2, &regSelect);
	union1(regSelect,&regSelect);

	// 如果模板没匹配上，把花纹和字符开掉 [2017.4 MJ]
	if ((pBInReg.bChar) && (oBInReg.ModelID >= 0) && (bModelMatch == false))
	{
		Hobject objTemp,objResult;
		connection(regSelect,&regSelect);
		select_shape(regSelect, &regSelect, "area", "and", 1, 99999999);
		count_obj(regSelect, &nNum);
		if (nNum > 0)
		{
			copy_obj(regSelect,&regExcludeTemp,1,-1);
			gen_empty_obj(&objResult);
			opening_circle(regSelect, &objTemp, pBInReg.fOpenSize);
			select_shape(objTemp, &objResult, "area", "and", 1, 99999999);
			copy_obj(objResult, &regSelect, 1, -1); //regSelect.num>=1
			ExtExcludeDefect(rtnInfo,regExcludeTemp,regSelect,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pBInReg.strName,true);
		}
	}

	select_shape(regDyn,&regDyn,"area","and",1,99999999);
	
	// 排除字符缺陷-排除浅边缘（双尺度计算）
	if (pBInReg.bDoubleScale)
	{
		Hobject objSel, objTemp, objResult, imgTempSrc, imgTempMean;
		gen_empty_obj(&objResult);
		dyn_threshold(imgReduce, imgMean, &regThresh1, pBInReg.nEdgeMax, "dark");
		connection(regThresh1, &regThresh1);
		select_shape(regThresh1, &regThresh1, "area", "and", pBInReg.nAreaMin, 99999999);
		count_obj(regThresh1, &nNum);
		if (nNum > 0)
		{
			for (i=1;i<nNum+1;i++)
			{
				select_obj(regThresh1, &objSel, i);
				dilation_circle(objSel, &objTemp, 51);
				reduce_domain(imgSrc, objTemp, &imgTempSrc);
				reduce_domain(imgMean, objTemp, &imgTempMean);
				dyn_threshold(imgTempSrc, imgTempMean, &regThresh2, pBInReg.nEdgeMin, "dark");
				connection(regThresh2, &regThresh2);
				count_obj(regThresh2, &nTemp);
				if (nTemp > 0)
				{
					for (j=1;j<nTemp+1;j++)
					{
						select_obj(regThresh2, &objTemp, j);
						test_subset_region(objSel, objTemp, &nSubset);
						if (nSubset==1)
						{
							area_center(objSel , &nAreaMin, NULL, NULL);
							area_center(objTemp, &nAreaMax, NULL, NULL);
							if (nAreaMin*1.0/nAreaMax < pBInReg.fScaleRatio)
							{
								concat_obj(objResult, objTemp, &objResult);
							}
						}
					}
				}
			}
		}
		difference(regSelect,objResult,&regExcludeTemp);
		union1(regExcludeTemp,&regSelect);
		connection(regSelect,&regSelect);
		select_shape(regSelect,&regSelect,"area","and",1,99999999);
		concat_obj(objSpecial, objResult, &objSpecial);
	}
	
	// 排除外部干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(regValid,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regSelect,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
				case 0://完全不检
					difference(regSelect,regDisturb,&regSelect);
					concat_obj(regDisturb,objSpecial,&objSpecial);
					break;
				case 1://典型缺陷				
					intersection(regSelect,regDisturb,&regTemp);
					difference(regSelect,regDisturb,&regSelect);
					opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
					concat_obj(regSelect,regTemp,&regSelect);
					concat_obj(regDisturb,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regSelect,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					break;
				case 2://排除条纹
					{
						Hobject regStripe;
						// 采用独立参数提取排除
						if (pDistReg.bStripeSelf)
						{
							distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
						}
						else
						{
							intersection(regDisturb,regSelect,&regTemp);
							distRegStripe(pDistReg,regTemp,&regStripe);					
						}
						difference(regSelect,regStripe,&regSelect);
						concat_obj(regStripe,objSpecial,&objSpecial);
						ExtExcludeDefect(rtnInfo,regCopy,regSelect,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					}
					break;
				case 3://2017.4---排除气泡
					{
						Hobject regBubble;
						intersection(regDisturb,regSelect,&regTemp);
						distBubble(imgSrc,pDistReg,regTemp,&regBubble);	//提取要排除的气泡
						difference(regSelect,regBubble,&regSelect);
						concat_obj(regBubble,objSpecial,&objSpecial);
						ExtExcludeDefect(rtnInfo,regCopy,regSelect,EXCLUDE_CAUSE_BUBBLE,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					}
					break;
				default:
					break;
			}
		}
	}
	union1(objSpecial, &objSpecial);

	// 排除干扰-开运算 [3/29/2016 TT]
	if (pBInReg.bOpen)
	{
		Hobject objSel,objTemp,objResult;
		Hlong nAreaTemp;	
		copy_obj(regSelect,&regExcludeTemp,1,-1);
		connection(regSelect,&regSelect);
		count_obj(regSelect, &nNum);
		gen_empty_obj(&objResult);
		if (nNum > 0)
		{
			for (i=1;i<nNum+1;i++)
			{
				select_obj(regSelect, &objSel, i);
				opening_circle(objSel, &objTemp, pBInReg.fOpenSize);
				area_center(objTemp, &nAreaTemp, NULL, NULL);
				if (nAreaTemp > 0)
				{
					concat_obj(objResult, objSel, &objResult);
				}
			}
		}
		copy_obj(objResult, &regSelect, 1, -1);
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regSelect,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pBInReg.strName,true);
	}

	//排除干扰-排除弧线
	if (pBInReg.bArc)
	{
		Hobject objArc,objSel,objTemp,objResult;		
		gen_empty_obj(&objResult);
		int minEdge = min(pBInReg.nEdge1,pBInReg.nEdge2);
		int minArea = min(pBInReg.nArea1,pBInReg.nArea2);
		int minLen  = min(pBInReg.nLen1,pBInReg.nLen2);
		int arcEdge = pBInReg.nArcEdge>minEdge?minEdge:pBInReg.nArcEdge;//对比度要最小
		dyn_threshold(imgReduce, imgMean, &objArc, arcEdge, "dark");
		// 减去干扰区域 [3/26/2016 zhaodt]
		difference(objArc, objSpecial, &objArc);
		connection(objArc,&objArc);
		select_shape(objArc, &objArc, HTuple("area").Concat("ra"), "or", 
			HTuple(minArea).Concat(minLen/2), HTuple(99999999).Concat(99999));
		select_shape(objArc,&objArc,HTuple("circularity").Concat("compactness"),"and",
			HTuple(0).Concat(0.7),HTuple(0.07).Concat(9999));//根据圆度和致密度选出条纹状区域
		count_obj(objArc,&nNum);	
		if (nNum>0)
		{
			for (i=1;i<nNum+1;++i)
			{
				select_obj(objArc,&objSel,i);
				fill_up(objSel,&objSel);
				opening_circle(objSel,&objTemp,pBInReg.nArcWidth/2.f);//判断条纹宽度
				connection(objTemp,&objTemp);
				select_shape(objTemp,&objTemp,"area","and",5,99999999);
				count_obj(objTemp,&nTemp);
				if (nTemp == 0)
				{
					concat_obj(objSel,objResult,&objResult);
				}
			}
			difference(regSelect,objResult,&regExcludeTemp);
			ExtExcludeDefect(rtnInfo,regSelect,regExcludeTemp,EXCLUDE_CAUSE_ARC,EXCLUDE_TYPE_GENERALDEFEXTS,pBInReg.strName,true);
			union1(regExcludeTemp,&regSelect);
			connection(regSelect,&regSelect);
			select_shape(regSelect,&regSelect,"area","and",1,99999999);
		}		
	}

	//报错
	connection(regSelect,&regSelect);
	select_shape(regSelect,&regSelect,"area","and",1,99999999);
	count_obj(regSelect, &nNum);
	if (nNum > 0)
	{
		concat_obj(rtnInfo.regError,regSelect,&rtnInfo.regError);
		Hobject objFst;
		double Ra,Rb,Comp,Conv;
		select_shape_std(regSelect,&objFst,"max_area",70);//报面积最大的一个
		elliptic_axis(objFst,&Ra,&Rb,NULL);
		Rb = (Rb==0)?0.5:Rb;
		compactness(objFst,&Comp);
		convexity(objFst,&Conv);
		if (Ra/Rb > 4 || Comp > 5 )
		{
			rtnInfo.nType = ERROR_CRACK;
		}
		else if (Conv < 0.75)
		{
			rtnInfo.nType = ERROR_BUBBLE;
		}
		else
		{
			rtnInfo.nType = ERROR_SPOT;
		}
	}

	// 特殊缺陷-圆斑检测
	if (pBInReg.bSpot)
	{
		Hobject regRemove;
		count_obj(regDyn, &nNum);
		if (nNum >  0)
		{
			difference(regDyn, objSpecial, &regExcludeTemp);
			union1(regExcludeTemp, &regExcludeTemp);
			connection(regExcludeTemp, &regExcludeTemp);
			//fill_up(regExcludeTemp,&regExcludeTemp);  //2017.5:会把气泡、圆形花纹当圆斑误报
			select_shape(regExcludeTemp, &regExcludeTemp, "area", "and", pBInReg.nAreaSpot, 99999999);
			select_shape(regExcludeTemp, &regExcludeTemp, "circularity", "and", pBInReg.fSpotRound, 1.0);
			count_obj(regExcludeTemp, &nNum);
			gen_empty_obj (&regRemove);
			for(i=0; i<nNum; i++)
			{
				Hlong NumHoles;
				select_obj (regExcludeTemp, &RegSel, i+1);
				connect_and_holes (RegSel, NULL, &NumHoles); //2017.8-排除中空区域，例如模点等
				if (NumHoles > 0)
				{
					concat_obj (regRemove, RegSel, &regRemove);
				}
			}
			difference (regExcludeTemp, regRemove, &regExcludeTemp);
			select_shape(regExcludeTemp, &regExcludeTemp, "area", "and", 1, 99999999);
			count_obj(regExcludeTemp, &nNum);
			if (nNum >= pBInReg.nSpotNum)
			{
				concat_obj(rtnInfo.regError,regExcludeTemp,&rtnInfo.regError);
				rtnInfo.nType = ERROR_SPOT;
				return rtnInfo;
			}

			////2017.4---排除气泡
			//if (nNum>0 && m_bDisturb)
			//{
			//	for (i=0;i<m_vDistItemID.size();++i)
			//	{
			//		s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			//		s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			//		if (!pDistReg.bEnabled)
			//		{
			//			continue;
			//		}
			//		Hobject xldVal,regDisturb;
			//		select_shape_xld(oDistReg.oDisturbReg, &xldVal, "area", "and", 1, 99999999);
			//		count_obj(xldVal,&nNum);
			//		if (nNum==0)
			//		{
			//			continue;
			//		}
			//		gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//		//判断该干扰区域是否与圆斑有交集
			//		intersection(regExcludeTemp,regDisturb,&regDisturb);
			//		select_shape(regDisturb,&regDisturb,"area","and",1,999999);
			//		count_obj(regDisturb,&nNum);
			//		if (nNum==0)
			//		{
			//			continue;
			//		}
			//		//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			//		Hobject regCopy;
			//		if (m_bExtExcludeDefect)
			//		{
			//			copy_obj(regExcludeTemp,&regCopy,1,-1);
			//		}
			//		//根据检测区域类型排除干扰
			//		if(pDistReg.nRegType == 3)
			//		{
			//			Hobject regBubble;
			//			distBubble(imgSrc,pDistReg,regDisturb,&regBubble);	//提取要排除的气泡
			//			difference(regExcludeTemp,regBubble,&regExcludeTemp);
			//			ExtExcludeDefect(rtnInfo,regCopy,regExcludeTemp,EXCLUDE_CAUSE_BUBBLE,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
			//	    }
			//	}

			//	select_shape(regExcludeTemp, &regExcludeTemp, "area", "and", 1, 99999999);
			//	count_obj(regExcludeTemp, &nNum);
			//	if (nNum >= pBInReg.nSpotNum)
			//	{
			//		concat_obj(rtnInfo.regError,regExcludeTemp,&rtnInfo.regError);
			//		rtnInfo.nType = ERROR_SPOT;
			//		return rtnInfo;
			//	}
			//}			
		}
	}

	//特殊缺陷-瓶底条纹
	if (pBInReg.bStrip)
	{
		Hobject regThreshPolar,regCon,regSel,regTemp,ErrorObj;
		Hobject imgPolar,imgPlMean;
		int nExpand = 0;
		int i;
		double dOriRow = currentOri.Row;
		double dOriCol = currentOri.Col;
		double nDist = dOutRadius-dInRadius+2*nExpand;
		Hlong Row1,Col1,Row2,Col2;
		double dRab,dPhi;
		gen_empty_obj(&ErrorObj);
		//极坐标系下分割外环区域
		if (pBInReg.nStripEdge >  0)
		{
			//以该灰度值提取，并做横向平滑
			polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
			mean_image(imgPolar,&imgPlMean,31,1);
			dyn_threshold(imgPolar,imgPlMean,&regThreshPolar,pBInReg.nStripEdge,"dark");
		}
		else
		{
			//将之前提取的区域，进行变换
			polar_trans_region(regDyn,&regThreshPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		}
		//2014.9.24 排除干扰
		polar_trans_region(objSpecial,&regTemp,dOriRow,dOriCol,0,2*PI,
			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		difference(regThreshPolar,regTemp,&regThreshPolar);
		connection(regThreshPolar,&regCon);
		select_shape(regCon,&regCon,"area","and",pBInReg.nStripArea,99999999);
		count_obj(regCon,&nNum);
		gen_empty_obj(&regTemp);
		for (i=0;i<nNum;i++)
		{
			select_obj(regCon,&regSel,i+1);
			smallest_rectangle1(regSel,&Row1,&Col1,&Row2,&Col2);
			if ((Row2-Row1)>pBInReg.nStripHeight && (Col2-Col1)>pBInReg.nStripWidthL 
				&& (Col2 - Col1)<pBInReg.nStripWidthH)
			{
				concat_obj(regSel,regTemp,&regTemp);
			}
		}	
		ExtExcludePolarDefect(rtnInfo,regCon,regTemp,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_WIDTH_AND_HEIGHT,EXCLUDE_TYPE_BOTSTRIPE,pBInReg.strName,true);				
		count_obj(regTemp,&nNum);
		gen_empty_obj(&regCon);
		for (i=0;i<nNum;i++)
		{
			select_obj(regTemp,&regSel,i+1);
			elliptic_axis(regSel,NULL,NULL,&dPhi);
			tuple_deg(dPhi,&dPhi);
			if ((dPhi>pBInReg.nStripAngleL && dPhi<pBInReg.nStripAngleH)
				|| ((dPhi+180)>pBInReg.nStripAngleL && (dPhi+180)<pBInReg.nStripAngleH))
			{
				concat_obj(regSel,regCon,&regCon);
			}
		}	
		ExtExcludePolarDefect(rtnInfo,regTemp,regCon,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_BOTSTRIPE,pBInReg.strName,true);				
		count_obj(regCon,&nNum);
		for (i=0;i<nNum;i++)
		{
			select_obj(regCon,&regSel,i+1);
			eccentricity(regSel,&dRab,NULL,NULL);
			if (dRab > pBInReg.fStripRab)
			{
				concat_obj(regSel,ErrorObj,&ErrorObj);
			}
		}	
		ExtExcludePolarDefect(rtnInfo,regCon,ErrorObj,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_BOTSTRIPE,pBInReg.strName,true);				
		count_obj(ErrorObj,&nNum);
		if (nNum>0)
		{
			polar_trans_region_inv(ErrorObj,&ErrorObj,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
			concat_obj(rtnInfo.regError, ErrorObj, &rtnInfo.regError);
			rtnInfo.nType = ERROR_BOTTOM_STRIPE;
			return rtnInfo;
		}
	}

	return rtnInfo;
}
//*功能：瓶底中环区域
RtnInfo CCheck::fnBMiddleReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pBMidReg pBMidReg = para.value<s_pBMidReg>();
	s_oBMidReg oBMidReg = shape.value<s_oBMidReg>();
	if (!pBMidReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	double dInRadius,dOutRadius;
	Hobject imgReduce,imgMean;
	Hobject regThresh1,regThresh2,regCheck,regSelect,regGray,RegSel,regExcludeTemp;
	HTuple tpOperation1,tpOperation2;
	Hlong nNum;
	int i;
	double meanGray;
	smallest_circle(oBMidReg.oInCircle,NULL,NULL,&dInRadius);
	smallest_circle(oBMidReg.oOutCircle,NULL,NULL,&dOutRadius);
	if (dOutRadius<dInRadius+5)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
		return rtnInfo;		
	}
	difference(oBMidReg.oOutCircle,oBMidReg.oInCircle,&regCheck);
	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regCheck,&regCheck,"area","and",100,99999999);
	count_obj(regCheck,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	gen_empty_obj(&regThresh1);
	gen_empty_obj(&regThresh2);
	
	reduce_domain(imgSrc, regCheck, &imgReduce);
	mean_image(imgReduce, &imgMean, 51, 51);

	// 使用第一组条件分割
	dyn_threshold(imgReduce, imgMean, &regThresh1, pBMidReg.nEdge1, "dark");
	Hobject objSpecial,regTemp;
	gen_empty_obj(&objSpecial);
	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(regCheck,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh1,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regThresh1,regDisturb,&regThresh1);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
				case 1://典型缺陷				
					intersection(regThresh1,regDisturb,&regTemp);
					difference(regThresh1,regDisturb,&regThresh1);
					opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
					concat_obj(regThresh1,regTemp,&regThresh1);
					concat_obj(regDisturb,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh1,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					break;
				case 2://排除条纹
					{
						Hobject regStripe;
						//2013.9.16 nanjc 增独立参数提取排除
						if (pDistReg.bStripeSelf)
						{
							distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
						}
						else
						{
							intersection(regDisturb,regThresh1,&regTemp);
							distRegStripe(pDistReg,regTemp,&regStripe);					
						}
						difference(regThresh1,regStripe,&regThresh1);
						concat_obj(regStripe,objSpecial,&objSpecial);
						ExtExcludeDefect(rtnInfo,regCopy,regThresh1,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					}
					break;
				case 3://2017.4---排除气泡
					break;
				default:
					break;
			}
		}
	}
	if (pBMidReg.bOpen)
	{
		copy_obj(regThresh1,&regExcludeTemp,1,-1);
		opening_circle(regThresh1, &regThresh1, pBMidReg.fOpenSize);
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh1,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pBMidReg.strName,true);
	}
	//	closing_circle(regThresh1, &regThresh1, 3.5);
	connection(regThresh1, &regExcludeTemp);
	if (pBMidReg.nOperation1==0)
	{
		tpOperation1 = "or";
	}
	else if (pBMidReg.nOperation1==1)
	{
		tpOperation1 = "and";
	}
	select_shape(regExcludeTemp, &regThresh1, HTuple("area").Concat("ra"), tpOperation1, 
		HTuple(pBMidReg.nArea1).Concat(pBMidReg.nLen1/2), HTuple(999999).Concat(9999));	
	ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh1,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pBMidReg.strName,true);
	count_obj(regThresh1,&nNum);
	if (nNum>0)
	{
		gen_empty_obj(&regGray);
		for (i = 0; i < nNum; ++i)
		{
			select_obj(regThresh1, &RegSel, i+1);
			intensity(RegSel, imgReduce, &meanGray, NULL);
			if (meanGray < pBMidReg.nMeanGray1)
			{
				concat_obj(regGray, RegSel, &regGray);
			}
		}
		ExtExcludeDefect(rtnInfo,regThresh1,regGray,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pBMidReg.strName,true);
		copy_obj(regGray,&regThresh1,1,-1);
	}
	if (pBMidReg.nEdge2 != pBMidReg.nEdge1)
	{
		// 使用第二组参数分割
		dyn_threshold(imgReduce, imgMean, &regThresh2, pBMidReg.nEdge2, "dark");
		difference(regThresh2,objSpecial,&regThresh2);
		if (pBMidReg.bOpen)
		{
			copy_obj(regThresh2,&regExcludeTemp,1,-1);
			opening_circle(regThresh2, &regThresh2, pBMidReg.fOpenSize);
			ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh2,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pBMidReg.strName,true);
		}
		//	closing_circle(regThresh2, &regThresh2, 3.5);
		connection(regThresh2, &regExcludeTemp);
		if (pBMidReg.nOperation2==0)
		{
			tpOperation2 = "or";
		}
		else if (pBMidReg.nOperation2==1)
		{
			tpOperation2 = "and";
		}
		select_shape(regExcludeTemp, &regThresh2, HTuple("area").Concat("ra"), tpOperation2, 
			HTuple(pBMidReg.nArea2).Concat(pBMidReg.nLen2/2), HTuple(99999999).Concat(99999));		
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh2,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pBMidReg.strName,true);
		count_obj(regThresh2,&nNum);
		if (nNum>0)
		{
			gen_empty_obj(&regGray);
			for (i = 0; i < nNum; ++i)
			{
				select_obj(regThresh2, &RegSel, i+1);
				intensity(RegSel, imgReduce, &meanGray, NULL);
				if (meanGray < pBMidReg.nMeanGray2)
				{
					concat_obj(regGray, RegSel, &regGray);
				}
			}
			ExtExcludeDefect(rtnInfo,regThresh2,regGray,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pBMidReg.strName,true);
			copy_obj(regGray,&regThresh2,1,-1);
		}
	}

	concat_obj(regThresh1, regThresh2, &regSelect);	
	union1(regSelect,&regSelect);
	connection(regSelect,&regSelect);
	count_obj(regSelect, &nNum);
	if (nNum == 0)
	{
		return rtnInfo;
	}
	//排除弧线
	if (pBMidReg.bArc)
	{
		Hobject objArc,objSel,objTemp,objResult;
		Hlong nTemp;
		gen_empty_obj(&objResult);
		int minEdge = min(pBMidReg.nEdge1,pBMidReg.nEdge2);
		int minArea = min(pBMidReg.nArea1,pBMidReg.nArea2);
		int minLen  = min(pBMidReg.nLen1,pBMidReg.nLen2);
		int arcEdge = pBMidReg.nArcEdge>minEdge?minEdge:pBMidReg.nArcEdge;//对比度要最小
		dyn_threshold(imgReduce, imgMean, &objArc, arcEdge, "dark");
		connection(objArc,&objArc);
		select_shape(objArc, &objArc, HTuple("area").Concat("ra"), "or", 
			HTuple(minArea).Concat(minLen/2), HTuple(99999999).Concat(99999));
		select_shape(objArc,&objArc,HTuple("circularity").Concat("compactness"),"and",
			HTuple(0).Concat(0.7),HTuple(0.07).Concat(9999));//根据圆度和致密度选出条纹状区域
		count_obj(objArc,&nNum);	
		if (nNum>0)
		{
			for (i=1;i<nNum+1;++i)
			{
				select_obj(objArc,&objSel,i);
				fill_up(objSel,&objSel);
				opening_circle(objSel,&objTemp,pBMidReg.nArcWidth/2.f);//判断条纹宽度
				connection(objTemp,&objTemp);
				select_shape(objTemp,&objTemp,"area","and",5,99999999);
				count_obj(objTemp,&nTemp);
				if (nTemp == 0)
				{
					concat_obj(objSel,objResult,&objResult);
				}
			}
			difference(regSelect,objResult,&regExcludeTemp);
			ExtExcludeDefect(rtnInfo,regSelect,regExcludeTemp,EXCLUDE_CAUSE_ARC,EXCLUDE_TYPE_GENERALDEFEXTS,pBMidReg.strName,true);
			union1(regExcludeTemp,&regSelect);
			connection(regSelect,&regSelect);
			select_shape(regSelect,&regSelect,"area","and",1,99999999);
		}		
	}
	//报错
	count_obj(regSelect, &nNum);
	if (nNum>0)
	{
		concat_obj(rtnInfo.regError,regSelect,&rtnInfo.regError);
		Hobject objFst;
		double Ra,Rb,Comp,Conv;
		select_shape_std(regSelect,&objFst,"max_area",70);//报面积最大的一个
		elliptic_axis(objFst,&Ra,&Rb,NULL);
		Rb = (Rb==0)?0.5:Rb;
		compactness(objFst,&Comp);
		convexity(objFst,&Conv);
		if (Ra/Rb > 4 || Comp > 5 )
		{
			rtnInfo.nType = ERROR_CRACK;
		}
		else if (Conv < 0.75)
		{
			rtnInfo.nType = ERROR_BUBBLE;
		}
		else
		{
			rtnInfo.nType = ERROR_SPOT;
		}
	}

	return rtnInfo;
}
//*功能：瓶底外环区域
RtnInfo CCheck::fnBOutterReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pBOutReg pBOutReg = para.value<s_pBOutReg>();
	s_oBOutReg oBOutReg = shape.value<s_oBOutReg>();
	if (!pBOutReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject regCheck,regThresh,regDyn,regExcludeTemp;
	Hobject imgReduce,imgMean;
	Hlong nNum;
	double dInRadius,dOutRadius;
	smallest_circle(oBOutReg.oInCircle,NULL,NULL,&dInRadius);
	smallest_circle(oBOutReg.oOutCircle,NULL,NULL,&dOutRadius);
	if (dOutRadius<dInRadius+5)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
		return rtnInfo;		
	}
	difference(oBOutReg.oOutCircle,oBOutReg.oInCircle,&regCheck);
	clip_region(regCheck,&regCheck,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regCheck,&regCheck,"area","and",100,99999999);
	count_obj(regCheck,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	reduce_domain(imgSrc,regCheck,&imgReduce);
	mean_image(imgSrc,&imgMean,75,75);//51->75
	dyn_threshold(imgReduce,imgMean,&regDyn,pBOutReg.nEdge,"dark");
	fast_threshold(imgReduce,&regThresh,0,pBOutReg.nGray,20);
	union2(regThresh,regDyn,&regThresh);
	int i;
	Hobject objSpecial,regTemp;
	gen_empty_obj(&objSpecial);
	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(regCheck,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
				case 0://完全不检
					difference(regThresh,regDisturb,&regThresh);
					concat_obj(regDisturb,objSpecial,&objSpecial);
					break;
				case 1://典型缺陷				
					intersection(regThresh,regDisturb,&regTemp);
					difference(regThresh,regDisturb,&regThresh);
					opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
					concat_obj(regThresh,regTemp,&regThresh);
					concat_obj(regDisturb,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					break;
				case 2://排除条纹
					{
						Hobject regStripe;
						//2013.9.16 nanjc 增独立参数提取排除
						if (pDistReg.bStripeSelf)
						{
							distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
						}
						else
						{
							intersection(regDisturb,regThresh,&regTemp);
							distRegStripe(pDistReg,regTemp,&regStripe);					
						}
						difference(regThresh,regStripe,&regThresh);
						concat_obj(regStripe,objSpecial,&objSpecial);
						ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
					}
					break;
				case 3://2017.4---排除瓶底气泡
					break;
				default:
					break;
			}
		}
	}

	//// 2017---排除干扰-内部干扰设置---MJ
	//if (pBOutReg.bExcArc)
	//{
	//	Hobject regBlob,regDisturb,regDiff;
	//	gen_empty_obj(&regTemp);
	//	int nPhiL,nPhiH;
	//	nPhiL = pBOutReg.nArcAngleL;
	//	nPhiH = pBOutReg.nArcAngleH;

	//	union1(regThresh, &regBlob);
	//	connection(regBlob, &regDisturb);
	//	if (nPhiL <= nPhiH)
	//	{
	//		if (nPhiL >= 90 && nPhiH > 90)
	//		{
	//			select_shape(regDisturb, &regDisturb, HTuple("phi"), "or", HTuple(0.0175*(nPhiL-180)), HTuple(0.0175*(nPhiH-180)));

	//		}
	//		else if (nPhiL <= 90 && nPhiH >= 90 )
	//		{
	//			select_shape(regDisturb, &regDisturb, HTuple("phi").Concat("phi"), "or", HTuple(PI*nPhiL/180).Concat(-PI/2), HTuple(PI/2).Concat(PI*(nPhiH-180)/180));

	//		}
	//		else
	//		{
	//			select_shape(regDisturb, &regDisturb, HTuple("phi"), "or", HTuple(0.0175*nPhiL), HTuple(0.0175*nPhiH));
	//		}

	//	}
	//	select_shape(regDisturb,&regDisturb,"inner_radius","and",0.5*pBOutReg.nArcInnerRadiusL,0.5*pBOutReg.nArcInnerRadiusH);
	//	select_shape(regDisturb,&regDisturb,HTuple("anisometry").Concat("anisometry"),"or",HTuple(pBOutReg.fArcAnisometryL).Concat(0),HTuple(pBOutReg.fArcAnisometryH).Concat(0));//长宽比

	//	concat_obj(regTemp, regDisturb, &regTemp);
	//	difference(regBlob, regDisturb, &regDiff);
	//	ExtExcludeDefect(rtnInfo,regBlob,regDiff,EXCLUDE_CAUSE_ARC,EXCLUDE_TYPE_GENERALDEFEXTS,pBOutReg.strName,true);	
	//		
	//    // 得到真正的检测区域
	//	difference(regThresh, regTemp, &regThresh);
	//}

	opening_circle(regThresh,&regDyn,pBOutReg.fOpenSize);
	//ExtExcludeDefect(rtnInfo,regThresh,regDyn,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_GENERALDEFEXTS,pBOutReg.strName,true);
	connection(regDyn,&regDyn);
	fill_up(regDyn,&regExcludeTemp);
	HTuple tpOperation;
	if (pBOutReg.nOperation==0)
	{
		tpOperation = "or";
	}
	else if (pBOutReg.nOperation==1)
	{
		tpOperation = "and";
	}
	select_shape(regExcludeTemp, &regDyn, HTuple("area").Concat("ra"), tpOperation, 
		HTuple(pBOutReg.nArea).Concat(pBOutReg.nLen/2), HTuple(99999999).Concat(pBOutReg.nLenMax/2));	
	ExtExcludeDefect(rtnInfo,regExcludeTemp,regDyn,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pBOutReg.strName,true);
	count_obj(regDyn,&nNum);
	if (nNum>0)
	{
		concat_obj(rtnInfo.regError,regDyn,&rtnInfo.regError);
		Hobject objFst;
		double Ra,Rb,Comp,Conv;
		select_shape_std(regDyn,&objFst,"max_area",70);//报面积最大的一个
		elliptic_axis(objFst,&Ra,&Rb,NULL);
		Rb = (Rb==0)?0.5:Rb;
		compactness(objFst,&Comp);
		convexity(objFst,&Conv);
		if (Ra/Rb > 4 || Comp > 5 )
		{
			rtnInfo.nType = ERROR_CRACK;
		}
		else if (Conv < 0.75)
		{
			rtnInfo.nType = ERROR_BUBBLE;
		}
		else
		{
			rtnInfo.nType = ERROR_SPOT;
		}
		return  rtnInfo;
	}
	//2014.1.6,灌装线检测瓶底条纹（瓶底掉落的牙签等杂物，由于有防滑带的干扰，仅检测角度）
	if (pBOutReg.bStrip)
	{
		Hobject regThreshPolar,regCon,regSel,regTemp,ErrorObj;
		Hobject imgPolar,imgPlMean;
		int nExpand = 0;
		int i;
		double dOriRow = currentOri.Row;
		double dOriCol = currentOri.Col;
		double nDist = dOutRadius-dInRadius+2*nExpand;
		Hlong Row1,Col1,Row2,Col2;
		double dRab,dPhi;
		gen_empty_obj(&ErrorObj);
		//极坐标系下分割外环区域
		if (pBOutReg.nStripEdge >  0)
		{
			//以该灰度值提取，并做横向平滑
			polar_trans_image_ext(imgSrc,&imgPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
			mean_image(imgPolar,&imgPlMean,31,1);
			dyn_threshold(imgPolar,imgPlMean,&regThreshPolar,pBOutReg.nStripEdge,"dark");
		}
		else
		{
			//将之前提取的区域，进行变换
			polar_trans_region(regThresh,&regThreshPolar,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		}
		//2014.9.24 排除干扰
		polar_trans_region(objSpecial,&regTemp,dOriRow,dOriCol,0,2*PI,
			dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,"nearest_neighbor");
		difference(regThreshPolar,regTemp,&regThreshPolar);
		connection(regThreshPolar,&regCon);
		select_shape(regCon,&regCon,"area","and",pBOutReg.nStripArea,99999999);
		count_obj(regCon,&nNum);
		gen_empty_obj(&regTemp);
		for (i=0;i<nNum;i++)
		{
			select_obj(regCon,&regSel,i+1);
			smallest_rectangle1(regSel,&Row1,&Col1,&Row2,&Col2);
			if ((Row2-Row1)>pBOutReg.nStripHeight && (Col2-Col1)>pBOutReg.nStripWidthL 
				&& (Col2 - Col1)<pBOutReg.nStripWidthH)
			{
				concat_obj(regSel,regTemp,&regTemp);
			}
		}	
		ExtExcludePolarDefect(rtnInfo,regCon,regTemp,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_WIDTH_AND_HEIGHT,EXCLUDE_TYPE_BOTSTRIPE,pBOutReg.strName,true);				
		count_obj(regTemp,&nNum);
		gen_empty_obj(&regCon);
		for (i=0;i<nNum;i++)
		{
			select_obj(regTemp,&regSel,i+1);
			elliptic_axis(regSel,NULL,NULL,&dPhi);
			tuple_deg(dPhi,&dPhi);
			if ((dPhi>pBOutReg.nStripAngleL && dPhi<pBOutReg.nStripAngleH)
				|| ((dPhi+180)>pBOutReg.nStripAngleL && (dPhi+180)<pBOutReg.nStripAngleH))
			{
				concat_obj(regSel,regCon,&regCon);
			}
		}	
		ExtExcludePolarDefect(rtnInfo,regTemp,regCon,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_BOTSTRIPE,pBOutReg.strName,true);				
		count_obj(regCon,&nNum);
		for (i=0;i<nNum;i++)
		{
			select_obj(regCon,&regSel,i+1);
			eccentricity(regSel,&dRab,NULL,NULL);
			if (dRab > pBOutReg.fStripRab)
			{
				concat_obj(regSel,ErrorObj,&ErrorObj);
			}
		}	
		ExtExcludePolarDefect(rtnInfo,regCon,ErrorObj,dInRadius-nExpand,dOutRadius+nExpand,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_BOTSTRIPE,pBOutReg.strName,true);				
		count_obj(ErrorObj,&nNum);
		if (nNum>0)
		{
			polar_trans_region_inv(ErrorObj,&ErrorObj,dOriRow,dOriCol,0,2*PI,
				dInRadius-nExpand,dOutRadius+nExpand,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
			concat_obj(rtnInfo.regError, ErrorObj, &rtnInfo.regError);
			rtnInfo.nType = ERROR_BOTTOM_STRIPE;
			return rtnInfo;
		}
	}
	return  rtnInfo;
}
//*功能：瓶身应力区域
RtnInfo CCheck::fnSSidewallReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSSideReg pSSideReg = para.value<s_pSSideReg>();
	s_oSSideReg oSSideReg = shape.value<s_oSSideReg>();
	if (!pSSideReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject regValid,regDyn,regThresh,regError,regSel,regDila,regExcludeTemp;
	Hobject imgReduce,imgMean;
	Hlong nNum;
	double dDilaSize = 3.5;
	if (pSSideReg.nShapeType == 0)
	{
		gen_region_contour_xld(oSSideReg.oCheckRegion,&regValid,"filled");
	} 
	else
	{
		gen_region_contour_xld(oSSideReg.oCheckRegion_Rect,&regValid,"filled");
	}	
	clip_region(regValid,&regValid,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regValid,&regValid,"area","and",100,99999999);
	count_obj(regValid,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INNER_STRESS;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}

	reduce_domain(imgSrc,regValid,&imgReduce);
	mean_image(imgSrc,&imgMean,51,51);
	dyn_threshold(imgReduce,imgMean,&regDyn,pSSideReg.nEdge,"light");
	threshold(imgReduce,&regThresh,pSSideReg.nGray,255);
	union2(regDyn,regThresh,&regDyn);
	closing_circle(regDyn,&regDyn,3.5);
	connection(regDyn,&regDyn);
	select_shape(regDyn,&regExcludeTemp,HTuple("area"),"and",HTuple(pSSideReg.nArea),HTuple(99999999));
	ExtExcludeDefect(rtnInfo,regDyn,regExcludeTemp,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideReg.strName,true);
	if (pSSideReg.fRab >= 1)
	{
		if(pSSideReg.bInspItem == 0)
		{
			select_shape(regExcludeTemp,&regDyn,HTuple("anisometry"),"and",HTuple(1),HTuple(pSSideReg.fRab));
			ExtExcludeDefect(rtnInfo,regExcludeTemp,regDyn,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideReg.strName,true);
		}
		else if(pSSideReg.bInspItem == 1)
		{
			 select_shape(regExcludeTemp,&regDyn,HTuple("anisometry"),"and",HTuple(pSSideReg.fRab),HTuple(9999999)); 
			 ExtExcludeDefect(rtnInfo,regExcludeTemp,regDyn,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideReg.strName,true);
		}

	}
	
	//2017.5 取消平均灰度判断，缺陷灰度值较小时，提取不出
	//count_obj(regDyn,&nNum);	
	//if (nNum>0)
	//{
	//	//判断与周围背景的灰度差
	//	gen_empty_obj(&regError);
	//	for (i=1;i<nNum+1;++i)
	//	{
	//		select_obj(regDyn,&regSel,i);
	//		dilation_circle(regSel,&regDila,dDilaSize);
	//		difference(regDila,regSel,&regDila);
	//		intensity(regSel,imgSrc,&dMeanSel,NULL);
	//		intensity(regDila,imgSrc,&dMeanDila,NULL);
	//		if ((dMeanSel-dMeanDila)>pSSideReg.nEdge)
	//		{
	//			concat_obj(regError,regSel,&regError);
	//		}
	//	}
	//}
	//ExtExcludeDefect(rtnInfo,regDyn,regError,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideReg.strName,true);
	//select_shape(regError,&regExcludeTemp,"area","and",1,999999);
		//排除干扰-内部干扰设置
	Hobject	regDisturb,regDiff;
	//gen_empty_obj(&objSpecial);
	if (pSSideReg.bDistCon1 || pSSideReg.bDistCon2)
	{
		Hobject regStripe,regSel,regTemp;
		double Phi;
		gen_empty_obj(&regTemp);
		 
		select_shape(regDyn, &regDisturb, "area", "and", 10, 9999999); //条纹至少得>10 pix
		//count_obj(regDisturb,&nNum);

		if (pSSideReg.bDistCon1) //排除垂直条纹
		{
			gen_empty_obj(&regStripe);
			closing_circle(regDisturb,&regDisturb,3);  //2018.3-区分竖直薄皮气泡与条纹
			select_shape(regDisturb, &regDisturb, "inner_radius", "or", 0.5*pSSideReg.nDistInRadiusL1,0.5*pSSideReg.nDistInRadiusH1);
			count_obj(regDisturb,&nNum);
		
			for (int i=0;i<nNum;i++)
			{
				select_obj(regDisturb,&regSel,i+1);
				elliptic_axis(regSel, NULL, NULL, &Phi);
				tuple_deg(Phi,&Phi);
				Phi=Phi<0?Phi+180.f:Phi;
				if ( fabs(90.f-Phi)<pSSideReg.nDistVerPhi)
				{
					concat_obj(regStripe,regSel,&regStripe);
				}
			}

			if (pSSideReg.nDistInRadiusH1>=pSSideReg.nDistInRadiusL1 && pSSideReg.nDistAniH1>=pSSideReg.nDistAniL1)
			{
				select_shape(regStripe,&regStripe,"inner_radius","and",0.5*pSSideReg.nDistInRadiusL1,0.5*pSSideReg.nDistInRadiusH1);
				select_shape(regStripe,&regStripe,HTuple("anisometry").Concat("anisometry"),"or",HTuple(pSSideReg.nDistAniL1).Concat(0),HTuple(pSSideReg.nDistAniH1).Concat(0));//长宽比
				concat_obj(regTemp,regStripe,&regTemp);
			}
			//difference(regDyn, regStripe, &regDiff);
			//ExtExcludeDefect(rtnInfo,regDyn, regDiff ,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_DISTCON1,pSSideReg.strName);	
		}
		
		if (pSSideReg.bDistCon2) //排除水平条纹
		{
            gen_empty_obj(&regStripe);
			count_obj(regDisturb,&nNum);
			for (int i=0;i<nNum;i++)
			{
				select_obj(regDisturb,&regSel,i+1);
				elliptic_axis(regSel, NULL, NULL, &Phi);
				tuple_deg(Phi,&Phi);
				if ( fabs(Phi)<pSSideReg.nDistHorPhi )
				{
					concat_obj(regStripe,regSel,&regStripe);
				}
			}
			if (pSSideReg.nDistInRadiusH2>=pSSideReg.nDistInRadiusL2 && pSSideReg.nDistAniH2>=pSSideReg.nDistAniL2)
			{
				select_shape(regStripe,&regStripe,"inner_radius","and",0.5*pSSideReg.nDistInRadiusL2,0.5*pSSideReg.nDistInRadiusH2);
				select_shape(regStripe,&regStripe,HTuple("anisometry").Concat("anisometry"),"or",HTuple(pSSideReg.nDistAniL2).Concat(0),HTuple(pSSideReg.nDistAniH2).Concat(0));//长宽比
				concat_obj(regTemp, regStripe, &regTemp);
			}
		}
		//concat_obj(objSpecial, regTemp, &objSpecial); //2017.5---将条纹添加到排除干扰中
	
		difference(regDyn, regTemp, &regDiff);
		select_shape(regDiff,&regDiff,"area","and",1,9999999);
		ExtExcludeDefect(rtnInfo,regDyn,regDiff,EXCLUDE_CAUSE_PHI,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideReg.strName,true);
		connection(regDiff,&regDyn);
	}
	
	//if (flg<1)
	{
		select_shape_proto(regDyn, regValid, &regError, "distance_contour", pSSideReg.nSideDistance, 9999);
		ExtExcludeDefect(rtnInfo,regDyn,regError,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_GENERALDEFEXTS,pSSideReg.strName,true);

		//count_obj(regError,&nNum);
		//QString s10 = QString::number(int(nNum), 10);
		//QString s20 = QString("%1%2").arg(s10).arg("regErrort_num");
		//writeAlgLog(s20);
		//printf("%s%s","名字：", pSSideReg.strName.toStdString().data() );
		if (nNum>0)
		{
			concat_obj(regError,rtnInfo.regError,&rtnInfo.regError);
			/*write_object(regError,".\\regError-n.hobj");*/
			select_shape(regError,&regSel,HTuple("area").Concat("ra"),"or",
				HTuple(pSSideReg.nArea*10).Concat(50),HTuple(99999999).Concat(99999));
			count_obj(regSel,&nNum);
			if (nNum>0)
			{
				if(pSSideReg.bInspItem==0)
					rtnInfo.nType = ERROR_INNER_STRESS;
				else
				{
					rtnInfo.nType = ERROR_CRACK; 
					rtnInfo.strErrorNm = pSSideReg.strName;
					writeAlgLog(rtnInfo.strErrorNm);
				}
			}
			else
			{
				if(pSSideReg.bInspItem == 0)
				rtnInfo.nType = ERROR_STONE_STRESS;
				else 
				{
					rtnInfo.nType = ERROR_CRACK;
					rtnInfo.strErrorNm =  pSSideReg.strName;
					writeAlgLog(rtnInfo.strErrorNm);
				}
			}
		}
	}
	return rtnInfo;
}
void CCheck::writeAlgLog(QString str)//QString *str
{
   // char tmpLog[256]=".\\lxnt_dlg.txt";
	QFile logFile("logdlg.txt");
	if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
	{
		return;
	}
	QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");   
	QString message = QString("%1 --> %2").arg(current_date_time).
		arg(str);  
	QTextStream textStream(&logFile);  
	textStream << message << "\r\n";  
	logFile.flush();  
	logFile.close();
	
}
//*功能：瓶底应力区域
RtnInfo CCheck::fnSBaseReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSBaseReg pSBaseReg = para.value<s_pSBaseReg>();
	s_oSBaseReg oSBaseReg = shape.value<s_oSBaseReg>();
	if (!pSBaseReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject regValid,regDyn,regThresh,regError,regSel,regDila,regExcludeTemp;
	Hobject imgReduce,imgMean;
	Hlong nNum;
	int i;
	double dDilaSize = 3.5;//,dMeanSel,dMeanDila;
	double dInRadius,dOutRadius;
	switch(pSBaseReg.nMethodIdx)
	{
	case 0:
		copy_obj(oSBaseReg.oOutCircle,&regValid,1,-1);
		break;
	case 1:
		smallest_circle(oSBaseReg.oInCircle,NULL,NULL,&dInRadius);
		smallest_circle(oSBaseReg.oOutCircle,NULL,NULL,&dOutRadius);
		if (dOutRadius<dInRadius+5)
		{
			gen_rectangle1(&rtnInfo.regError,120,20,220,120);
			rtnInfo.nType = ERROR_INNER_STRESS;
			rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
			return rtnInfo;		
		}
		difference(oSBaseReg.oOutCircle, oSBaseReg.oInCircle, &regValid);	
		break;
	default:
		gen_empty_obj(&regValid);
		break;
	}
	reduce_domain(imgSrc,regValid,&imgReduce);
	mean_image(imgSrc,&imgMean,51,51);
	dyn_threshold(imgReduce,imgMean,&regDyn,pSBaseReg.nEdge,"light");
	threshold(imgReduce,&regThresh,pSBaseReg.nGray,255);
	union2(regDyn,regThresh,&regDyn);
	closing_circle(regDyn,&regDyn,3.5);
	Hobject objSpecial;
	gen_empty_obj(&objSpecial);
	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(regValid,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regDyn,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regDyn,regDisturb,&regDyn);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
			default:
				break;
			}
		}
	}
	connection(regDyn,&regDyn);
	select_shape(regDyn,&regExcludeTemp,HTuple("area"),"and",HTuple(pSBaseReg.nArea),HTuple(99999999));
	ExtExcludeDefect(rtnInfo,regDyn,regExcludeTemp,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSBaseReg.strName,true);
	if (pSBaseReg.fRab >= 1)
	{
		select_shape(regExcludeTemp,&regDyn,HTuple("anisometry"),"and",HTuple(1),HTuple(pSBaseReg.fRab));
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regDyn,EXCLUDE_CAUSE_ANISOMETRY,EXCLUDE_TYPE_GENERALDEFEXTS,pSBaseReg.strName,true);
	}
	//count_obj(regDyn,&nNum);	
	//if (nNum>0)
	//{
	//	//判断与周围背景的灰度差
	//	gen_empty_obj(&regError);
	//	for (i=1;i<nNum+1;++i)
	//	{
	//		select_obj(regDyn,&regSel,i);
	//		dilation_circle(regSel,&regDila,dDilaSize);
	//		difference(regDila,regSel,&regDila);
	//		intensity(regSel,imgSrc,&dMeanSel,NULL);
	//		intensity(regDila,imgSrc,&dMeanDila,NULL);
	//		if ((dMeanSel-dMeanDila)>pSBaseReg.nEdge)
	//		{
	//			concat_obj(regError,regSel,&regError);
	//		}
	//	}
	//}
	//ExtExcludeDefect(rtnInfo,regDyn,regError,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_GENERALDEFEXTS,pSBaseReg.strName,true);
	//select_shape(regError,&regExcludeTemp,"area","and",1,999999);
	//select_shape_proto(regExcludeTemp, regValid, &regError, "distance_contour", pSBaseReg.nSideDistance, 9999);
	//ExtExcludeDefect(rtnInfo,regExcludeTemp,regError,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_GENERALDEFEXTS,pSBaseReg.strName,true);
	//2014.10.27 取消平均灰度判断，缺陷在瓶底亮条区域时，否则无法提取
	select_shape_proto(regDyn, regValid, &regError, "distance_contour", pSBaseReg.nSideDistance, 9999);
	ExtExcludeDefect(rtnInfo,regDyn,regError,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_GENERALDEFEXTS,pSBaseReg.strName,true);
	count_obj(regError,&nNum);
	if (nNum>0)
	{
		concat_obj(regError,rtnInfo.regError,&rtnInfo.regError);
		select_shape(regError,&regSel,HTuple("area").Concat("ra"),"or",
			HTuple(pSBaseReg.nArea*10).Concat(50),HTuple(99999999).Concat(99999));
		count_obj(regSel,&nNum);
		if (nNum>0)
		{
			rtnInfo.nType = ERROR_INNER_STRESS;
		}
		else
		{
			rtnInfo.nType = ERROR_STONE_STRESS;
		}
	}

	return rtnInfo;
}
//*功能：瓶底凸起区域
RtnInfo CCheck::fnSBaseConvexReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSBaseConvexReg pSBaseConvexReg = para.value<s_pSBaseConvexReg>();
	s_oSBaseConvexReg oSBaseConvexReg = shape.value<s_oSBaseConvexReg>();
	if (!pSBaseConvexReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	gen_empty_obj(&oSBaseConvexReg.oBaseConvexRegion);
	gen_empty_obj(&oSBaseConvexReg.oValidRegion);
	shape.setValue(oSBaseConvexReg);
	if (!pSBaseConvexReg.bContDefects && !pSBaseConvexReg.bGenDefects)
	{
		return rtnInfo;
	}
	Hlong nNum;
	int i,j;
	double Ra,Rb,Comp,Conv;
	Hobject imgReduce,imgMean,imgSobel;
	Hobject regSobel,regLine,regRect,regDiff;
	Hobject regThresh,regBinThresh,regTemp,regBlob;
	Hobject objSpecial;//完全不检、典型缺陷及条纹区域
	Hlong Row1, Col1, Row2, Col2;
	HTuple tpRows,tpCols,tpIndices;
	int nMaskSizeWidth,nMaskSizeHeight;
	gen_empty_obj(&objSpecial);
	gen_region_contour_xld(oSBaseConvexReg.oCheckRegion,&oSBaseConvexReg.oValidRegion,"filled");
	clip_region(oSBaseConvexReg.oValidRegion,&oSBaseConvexReg.oValidRegion,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(oSBaseConvexReg.oValidRegion,&oSBaseConvexReg.oValidRegion,"area","and",100,99999999);
	count_obj(oSBaseConvexReg.oValidRegion,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}
	//检测轮廓
	//分块
	smallest_rectangle1(oSBaseConvexReg.oValidRegion,&Row1, &Col1, &Row2, &Col2);
	int nRegionWidth = Col2-Col1+1;
	int nRegionHeight = Row2-Row1;
	int nPartWidth = nRegionWidth;
	int nPartHeight = nRegionHeight/1 + 1;//分割2部分
	Hobject PartionRegion;			
	partition_rectangle(oSBaseConvexReg.oValidRegion, &PartionRegion, nPartWidth, nPartHeight);		
	connection(PartionRegion, &PartionRegion);		
	Hobject PartSel, tempRegion,regSel,edgeRegs;
	Hobject ImgPart;			
	Hlong nNumber;
	gen_empty_obj(&edgeRegs);
	select_shape(PartionRegion, &PartionRegion, "area", "and", 100, 99999999);
	count_obj(PartionRegion, &nNumber);
	for (i = 0; i< nNumber; ++i)
	{
		select_obj(PartionRegion, &PartSel, i+1); 		
		reduce_domain(imgSrc, PartSel, &ImgPart);	
		bin_threshold(ImgPart,&tempRegion);
		connection(tempRegion,&tempRegion);		
		select_shape(tempRegion,&tempRegion,HTuple("area"),"and",HTuple(10),HTuple(9999999));
		Hlong edgeNums;
		count_obj(tempRegion,&edgeNums);
		if (edgeNums==0)
			continue;
		HTuple edgeCol;
		area_center(tempRegion,NULL,NULL,&edgeCol);
		for (j=0;j<edgeNums;++j)
		{
			//排除靠边1/8
			if (edgeCol[j].I()>Col1+nPartWidth/8.0 && edgeCol[j].I()<Col2-nPartWidth/8.0)
			{
				select_obj(tempRegion,&regSel,j+1);
				concat_obj(edgeRegs, regSel, &edgeRegs);
			}
		}
	}
	union1(edgeRegs, &regTemp);
	fill_up_shape(regTemp,&regTemp,"area",0,100);
	connection (regTemp, &regTemp);
	select_shape_std(regTemp, &regThresh, "max_area", 70);
	//opening_circle(regThresh,&regTemp,pSBaseConvexReg.fOpeningSize);
	//connection (regTemp, &regTemp);
	erosion_circle(regThresh,&regTemp,pSBaseConvexReg.fOpeningSize);
	connection (regTemp, &regTemp);
	dilation_circle(regTemp,&regTemp,pSBaseConvexReg.fOpeningSize);
	select_shape_std(regTemp, &regSobel, "max_area", 70);
	count_obj(regSobel,&nNum);
	if (0==nNum)
	{
		//检测轮廓缺陷报凸起区域异常
		if (pSBaseConvexReg.bContDefects)
		{
			copy_obj(regThresh,&rtnInfo.regError,1,-1);
			rtnInfo.nType = ERROR_BASECONVEX_CONT;
			rtnInfo.strEx = QObject::tr("Failure to find the convex region!");//瓶底凸起区域查找错误!
			return rtnInfo;		
		}
	}
	else
	{
		copy_obj(regThresh,&oSBaseConvexReg.oBaseConvexRegion,1,-1);
		shape.setValue(oSBaseConvexReg);
		//提取列靠边区域，下侧靠边区域，其它缺陷区域
		Hobject regColSide,regRowSide,regOther;
		double dtempRow,dtempCol;
		Hlong nArea1,nArea2;
		gen_empty_obj(&regColSide);
		gen_empty_obj(&regRowSide);
		gen_empty_obj(&regOther);
		difference(regThresh,regSobel,&regDiff);
		connection(regDiff,&regDiff);
		count_obj(regDiff,&nNum);
		for (i = 0; i< nNum; ++i)
		{
			select_obj(regDiff,&regSel,i+1);
			area_center(regSel,NULL,&dtempRow,&dtempCol);
			//列靠边1/8
			if (dtempCol<Col1+nPartWidth/8.0 || dtempCol>Col2-nPartWidth/8.0)
			{
				concat_obj(regColSide, regSel, &regColSide);
				continue;
			}
			//行靠下边1/10
			if (dtempRow>Row2-nPartHeight/10.0)
			{
				concat_obj(regRowSide, regSel, &regRowSide);
				continue;
			}
			//其它
			concat_obj(regOther, regSel, &regOther);
		}
		union2(regSobel,regRowSide,&regSobel);
		//检测轮廓缺陷报凸起区域异常
		if (pSBaseConvexReg.bContDefects)
		{
			//提取的凸起区域过大,或过偏，提取错误
			connection(regSobel,&regTemp);
			select_shape_std(regTemp, &regTemp, "max_area", 70);
			area_center(regTemp,&nArea2,NULL,&dtempCol);
			area_center(oSBaseConvexReg.oValidRegion,&nArea1,NULL,NULL);
			if (nArea2>0.8*nArea1 || dtempCol<Col1+nPartWidth/5.0 || dtempCol>Col2-nPartWidth/5.0)
			{
				copy_obj(regTemp,&rtnInfo.regError,1,-1);
				rtnInfo.nType = ERROR_BASECONVEX_CONT;
				rtnInfo.strEx = QObject::tr("Failure to find the convex region!");//瓶底凸起区域查找错误!
				return rtnInfo;		
			}
			//其它区域判断
			connection(regOther,&regTemp);
			select_shape(regTemp,&regTemp,HTuple("area").Concat("ra"),"or",
				HTuple(pSBaseConvexReg.nContArea).Concat(pSBaseConvexReg.nContLength/2),
				HTuple(99999999).Concat(99999));
			count_obj(regTemp,&nNum);
			if (nNum>0)
			{
				copy_obj(regTemp,&rtnInfo.regError,1,-1);
				//copy_obj(regSobel,&oSBaseConvexReg.oBaseConvexRegion,1,-1);
				//shape.setValue(oSBaseConvexReg);
				rtnInfo.nType = ERROR_BASECONVEX_CONT;
				return rtnInfo;		
			}
		}
		//排除预处理区域中凸底区域
		union2(regSobel,regColSide,&regSobel);
		connection(regSobel,&regSobel);
		select_shape_std(regSobel, &regSobel, "max_area", 70);
		difference(oSBaseConvexReg.oValidRegion,regSobel,&oSBaseConvexReg.oValidRegion);
	}
	//检测区域预处理
	if (pSBaseConvexReg.bGenRoi)
	{
		s_ROIPara roiPara;
		roiPara.fRoiRatio = pSBaseConvexReg.fRoiRatio;
		roiPara.nClosingWH = pSBaseConvexReg.nClosingWH;
		roiPara.nGapWH = pSBaseConvexReg.nGapWH;
		rtnInfo = genValidROI(imgSrc,roiPara,oSBaseConvexReg.oValidRegion,&oSBaseConvexReg.oValidRegion);
		if (rtnInfo.nType>0)
		{
			return rtnInfo;
		}
	}
	shape.setValue(oSBaseConvexReg);
	//检测一般缺陷
	reduce_domain(imgSrc,oSBaseConvexReg.oValidRegion,&imgReduce);
	//2013.9.16 nanjc 预处理区域过小时，重新设定均值尺度
	//mean_image(imgReduce,&imgMean,pSGenReg.nMaskSize,pSGenReg.nMaskSize);
	smallest_rectangle1(oSBaseConvexReg.oValidRegion, &Row1, &Col1, &Row2, &Col2);	
	nMaskSizeWidth = min((Col2-Col1)/3,pSBaseConvexReg.nMaskSize);
	nMaskSizeWidth = max(1,nMaskSizeWidth);
	nMaskSizeHeight = min((Row2-Row1)/3,pSBaseConvexReg.nMaskSize);
	nMaskSizeHeight = max(1,nMaskSizeHeight);
	mean_image(imgReduce, &imgMean, nMaskSizeWidth,
		nMaskSizeHeight);

	dyn_threshold(imgReduce,imgMean,&regThresh,pSBaseConvexReg.nEdge,"dark");
	if (pSBaseConvexReg.nGray>0)
	{
		threshold(imgReduce,&regBinThresh,0,pSBaseConvexReg.nGray);
		union2(regBinThresh,regThresh,&regThresh);
	}
	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(oSBaseConvexReg.oValidRegion,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regThresh,regDisturb,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
			case 1://典型缺陷				
				intersection(regThresh,regDisturb,&regTemp);
				difference(regThresh,regDisturb,&regThresh);
				opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
				concat_obj(regThresh,regTemp,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				break;
			case 2://排除条纹
				{
					Hobject regStripe;
					//2013.9.16 nanjc 增独立参数提取排除
					if (pDistReg.bStripeSelf)
					{
						distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
					}
					else
					{
						intersection(regDisturb,regThresh,&regTemp);
						distRegStripe(pDistReg,regTemp,&regStripe);					
					}
					difference(regThresh,regStripe,&regThresh);
					concat_obj(regStripe,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				}
				break;
			default:
				break;
			}
		}
	}

	//分析缺陷类型
	union1(regThresh,&regThresh);
	connection(regThresh,&regThresh);
	select_shape(regThresh,&regBlob,HTuple("area").Concat("ra"),"or",
		HTuple(pSBaseConvexReg.nArea).Concat(pSBaseConvexReg.nLength/2),
		HTuple(99999999).Concat(99999));
	ExtExcludeDefect(rtnInfo,regThresh,regBlob,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pSBaseConvexReg.strName);
	count_obj(regBlob,&nNum);
	if (nNum>0)//报面积最大的一个
	{
		select_shape_std(regBlob,&regTemp,"max_area",70);
		elliptic_axis(regTemp,&Ra,&Rb,NULL);
		Rb = (Rb==0)?0.5:Rb;
		compactness(regTemp,&Comp);
		convexity(regTemp,&Conv);
		if (Ra/Rb > 4 || Comp >5)
		{
			rtnInfo.nType = ERROR_CRACK;
		}
		else if (Conv < 0.75)
		{
			rtnInfo.nType = ERROR_BUBBLE;
		}
		else 
		{
			rtnInfo.nType = ERROR_SPOT;
		}
		concat_obj(rtnInfo.regError,regBlob,&rtnInfo.regError);
		return rtnInfo;
	}	

	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：瓶底凸起区域（现场图像较次，效果不好，修改为上面的方法）
RtnInfo CCheck::fnSBaseConvexReg1(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSBaseConvexReg pSBaseConvexReg = para.value<s_pSBaseConvexReg>();
	s_oSBaseConvexReg oSBaseConvexReg = shape.value<s_oSBaseConvexReg>();
	gen_empty_obj(&oSBaseConvexReg.oBaseConvexRegion);
	if (!pSBaseConvexReg.bContDefects && !pSBaseConvexReg.bGenDefects)
	{
		shape.setValue(oSBaseConvexReg);
		return rtnInfo;
	}
	Hlong nNum;
	int i;
	double Ra,Rb,Comp,Conv;
	Hobject imgReduce,imgMean,imgSobel;
	Hobject regSobel,regLine,regRect,regDiff;
	Hobject regThresh,regBinThresh,regTemp,regBlob;
	Hobject objSpecial;//完全不检、典型缺陷及条纹区域
	Hlong Row1, Col1, Row2, Col2;
	HTuple tpRows,tpCols,tpIndices;
	gen_empty_obj(&objSpecial);
	gen_region_contour_xld(oSBaseConvexReg.oCheckRegion,&oSBaseConvexReg.oValidRegion,"filled");
	clip_region(oSBaseConvexReg.oValidRegion,&oSBaseConvexReg.oValidRegion,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(oSBaseConvexReg.oValidRegion,&oSBaseConvexReg.oValidRegion,"area","and",100,99999999);
	count_obj(oSBaseConvexReg.oValidRegion,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}
	//检测轮廓
	smallest_rectangle1(oSBaseConvexReg.oValidRegion, &Row1, &Col1, &Row2, &Col2);	
	reduce_domain(imgSrc,oSBaseConvexReg.oValidRegion,&imgReduce);
	sobel_amp (imgReduce, &imgSobel, "sum_abs", 3);
	threshold (imgSobel, &regSobel, pSBaseConvexReg.nContGray, 255);
	connection (regSobel, &regTemp);
	select_shape (regTemp, &regTemp, "width", "and", (Col2-Col1)/3, 99999);
	select_shape (regTemp, &regSobel, "height", "and", (Row2-Row1)/4, 99999);
	count_obj(regSobel,&nNum);
	if (nNum>0)
	{
		get_region_points (regSobel, &tpRows, &tpCols);	
		tuple_length (tpCols, &nNum);
		if (nNum>3)
		{
			tuple_sort_index (tpCols, &tpIndices);
			Hlong lIndex = tpRows[tpIndices[0].L()].L()<tpRows[tpIndices[nNum-1].L()].L()?0:nNum-1;
			Hlong lminRow = tpRows[tpIndices[lIndex].L()].L();
			gen_region_line(&regLine,lminRow,Col1,lminRow,Col2);
			union2(regLine,regSobel,&regSobel);
			fill_up(regSobel,&regSobel);
			gen_rectangle1(&regRect,Row1,Col1,lminRow,Col2);
			//若检测一般缺陷，去掉下侧区域防止误检,并去掉凸起区域
			if (pSBaseConvexReg.bGenDefects)
			{
				intersection (oSBaseConvexReg.oValidRegion, regRect, &oSBaseConvexReg.oValidRegion);
				difference(oSBaseConvexReg.oValidRegion, regSobel, &oSBaseConvexReg.oValidRegion);
			}
			//检测轮廓缺陷
			if (pSBaseConvexReg.bContDefects)
			{
				intersection (regSobel, regRect, &regSobel);
				s_pSideLoc pSideLoc;
				if (m_bLocate && vModelParas[m_nLocateItemID].canConvert<s_pSideLoc>())
				{		
					pSideLoc = vModelParas[m_nLocateItemID].value<s_pSideLoc>();
				}
				if (!m_bLocate || (pSideLoc.nMethodIdx != 0 && pSideLoc.nMethodIdx != 1 && pSideLoc.nMethodIdx != 5))
				{
					gen_empty_obj(&oSBaseConvexReg.oValidRegion);
					shape.setValue(oSBaseConvexReg);
					gen_rectangle1(&rtnInfo.regError,120,20,220,120);
					rtnInfo.nType = ERROR_BASECONVEX_CONT;
					rtnInfo.strEx = QObject::tr("The location method must be 'TranslationRotation'、'HorizontalTranslation' or 'CenterHorTranslation'!");
					return rtnInfo;		
				}
				opening_circle (regSobel, &regTemp, 2);
				connection(regTemp,&regTemp);
				select_shape (regTemp, &regTemp, "area", "and", 10, 99999999);
				select_shape_std(regTemp,&regSobel,"max_area",70);
				count_obj(regSobel,&nNum);
				if (nNum>0)
				{
					concat_obj(oSBaseConvexReg.oBaseConvexRegion,regSobel,&oSBaseConvexReg.oBaseConvexRegion);
					shape.setValue(oSBaseConvexReg);
					//旋转
					HTuple HomMat2DLine,HomMat2DLineInv;
					vector_angle_to_rigid(currentOri.Row,currentOri.Col,PI/2,currentOri.Row,
						currentOri.Col,currentOri.Angle,&HomMat2DLine);
					hom_mat2d_invert(HomMat2DLine, &HomMat2DLineInv);
					affine_trans_region(regSobel, &regSobel,HomMat2DLineInv, "false");
					affine_trans_region(regRect, &regRect,HomMat2DLineInv, "false");
					get_region_points (regSobel, &tpRows, &tpCols);	
					//计算中心纵坐标
					//tuple_length(tpRows,&nNum);
					//int itemp = 0;
					//int iCount = 0;
					//for (i = 0 ;i<nNum;++i)
					//{
					//	if (0==tpRows[i].L() -tpRows[0].L())
					//	{
					//		itemp += tpCols[i].L();
					//		iCount++;
					//	}
					//	else
					//		break;
					//}
					//double dMidCol = 1.0*itemp/iCount;
					double dMidCol;
					area_center(regSobel,NULL,NULL,&dMidCol);
					//移动
					double offCol = m_nWidth/2.0 - dMidCol;
					move_region(regSobel, &regSobel, HTuple(0), HTuple(offCol));				
					move_region(regRect, &regRect, HTuple(0), HTuple(offCol));				
					mirror_region(regSobel, &regTemp, "column", m_nWidth); 
					symm_difference(regTemp, regSobel, &regDiff);	
					opening_circle(regDiff,&regDiff,pSBaseConvexReg.fOpeningSize);
					connection(regDiff, &regDiff);
					select_shape(regDiff, &regDiff, "area", "and", pSBaseConvexReg.nContArea, 99999999);
					select_shape_proto(regDiff, regRect, &regDiff, "distance_contour",2, 10000);
					count_obj(regDiff, &nNum);
					if (nNum > 0)
					{	
						gen_empty_obj(&oSBaseConvexReg.oValidRegion);
						shape.setValue(oSBaseConvexReg);
						move_region(regDiff, &regDiff, 0, -offCol);
						affine_trans_region(regDiff, &regDiff, HomMat2DLine, "false");	
						concat_obj(rtnInfo.regError,regDiff,&rtnInfo.regError);
						rtnInfo.nType = ERROR_BASECONVEX_CONT;
						return rtnInfo;
					}
				}
			}
		}
	}

	//检测区域预处理
	if (pSBaseConvexReg.bGenRoi)
	{
		s_ROIPara roiPara;
		roiPara.fRoiRatio = pSBaseConvexReg.fRoiRatio;
		roiPara.nClosingWH = pSBaseConvexReg.nClosingWH;
		roiPara.nGapWH = pSBaseConvexReg.nGapWH;
		rtnInfo = genValidROI(imgSrc,roiPara,oSBaseConvexReg.oValidRegion,&oSBaseConvexReg.oValidRegion);
		if (rtnInfo.nType>0)
		{
			return rtnInfo;
		}
	}
	shape.setValue(oSBaseConvexReg);
	//检测一般缺陷
	reduce_domain(imgSrc,oSBaseConvexReg.oValidRegion,&imgReduce);
	//2013.9.16 nanjc 预处理区域过小时，重新设定均值尺度
	//mean_image(imgReduce,&imgMean,pSGenReg.nMaskSize,pSGenReg.nMaskSize);
	int nMaskSizeWidth,nMaskSizeHeight;
	smallest_rectangle1(oSBaseConvexReg.oValidRegion, &Row1, &Col1, &Row2, &Col2);	
	nMaskSizeWidth = min((Col2-Col1)/3,pSBaseConvexReg.nMaskSize);
	nMaskSizeWidth = max(1,nMaskSizeWidth);
	nMaskSizeHeight = min((Row2-Row1)/3,pSBaseConvexReg.nMaskSize);
	nMaskSizeHeight = max(1,nMaskSizeHeight);
	mean_image(imgReduce, &imgMean, nMaskSizeWidth,
		nMaskSizeHeight);

	dyn_threshold(imgReduce,imgMean,&regThresh,pSBaseConvexReg.nEdge,"dark");
	if (pSBaseConvexReg.nGray>0)
	{
		threshold(imgReduce,&regBinThresh,0,pSBaseConvexReg.nGray);
		union2(regBinThresh,regThresh,&regThresh);
	}
	//排除干扰
	if (m_bDisturb)
	{
		for (i=0;i<m_vDistItemID.size();++i)
		{
			s_pDistReg pDistReg = vModelParas[m_vDistItemID[i]].value<s_pDistReg>();
			s_oDistReg oDistReg = vModelShapes[m_vDistItemID[i]].value<s_oDistReg>();
			if (!pDistReg.bEnabled)
			{
				continue;
			}
			Hobject xldVal,regDisturb;
			//2015.2.2 针对Data base: object has no XLD-ID in operator gen_region_contour_xld 做此修改，是否有效未知
			if(pDistReg.nShapeType == 0)
			{
				copy_obj(oDistReg.oDisturbReg, &xldVal, 1, -1);
			}
			else
			{
				copy_obj(oDistReg.oDisturbReg_Rect, &xldVal, 1, -1);
			}
			select_shape_xld(xldVal, &xldVal, "area", "and", 1, 99999999);
			count_obj(xldVal,&nNum);
			if (nNum==0)
			{
				continue;
			}
			gen_region_contour_xld(xldVal,&regDisturb,"filled");
			//判断该干扰区域是否与检测区域有交集
			intersection(oSBaseConvexReg.oValidRegion,regDisturb,&regDisturb);
			select_shape(regDisturb,&regDisturb,"area","and",1,99999999);
			count_obj(regDisturb,&nNum);
			if (nNum==0)
			{
				continue;
			}
			//对排除干扰前的区域进行备份，用于计算每次排除掉的干扰区域
			Hobject regCopy;
			if (m_bExtExcludeDefect)
			{
				copy_obj(regThresh,&regCopy,1,-1);
			}
			//根据检测区域类型排除干扰
			switch(pDistReg.nRegType)
			{
			case 0://完全不检
				difference(regThresh,regDisturb,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				break;
			case 1://典型缺陷				
				intersection(regThresh,regDisturb,&regTemp);
				difference(regThresh,regDisturb,&regThresh);
				opening_circle(regTemp,&regTemp,pDistReg.fOpenSize);
				concat_obj(regThresh,regTemp,&regThresh);
				concat_obj(regDisturb,objSpecial,&objSpecial);
				ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_TYPICAL,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				break;
			case 2://排除条纹
				{
					Hobject regStripe;
					//2013.9.16 nanjc 增独立参数提取排除
					if (pDistReg.bStripeSelf)
					{
						distRegStripe(imgSrc,pDistReg,oDistReg,&regStripe);					
					}
					else
					{
						intersection(regDisturb,regThresh,&regTemp);
						distRegStripe(pDistReg,regTemp,&regStripe);					
					}
					difference(regThresh,regStripe,&regThresh);
					concat_obj(regStripe,objSpecial,&objSpecial);
					ExtExcludeDefect(rtnInfo,regCopy,regThresh,EXCLUDE_CAUSE_STRIPES,EXCLUDE_TYPE_DISTURB,pDistReg.strName,true);
				}
				break;
			default:
				break;
			}
		}
	}

	//分析缺陷类型
	union1(regThresh,&regThresh);
	connection(regThresh,&regThresh);
	select_shape(regThresh,&regBlob,HTuple("area").Concat("ra"),"or",
		HTuple(pSBaseConvexReg.nArea).Concat(pSBaseConvexReg.nLength/2),
		HTuple(99999999).Concat(99999));
	ExtExcludeDefect(rtnInfo,regThresh,regBlob,EXCLUDE_CAUSE_AREA_AND_LENGTH,EXCLUDE_TYPE_GENERALDEFEXTS,pSBaseConvexReg.strName);
	count_obj(regBlob,&nNum);
	if (nNum>0)//报面积最大的一个
	{
		select_shape_std(regBlob,&regTemp,"max_area",70);
		elliptic_axis(regTemp,&Ra,&Rb,NULL);
		Rb = (Rb==0)?0.5:Rb;
		compactness(regTemp,&Comp);
		convexity(regTemp,&Conv);
		if (Ra/Rb > 4 || Comp >5)
		{
			rtnInfo.nType = ERROR_CRACK;
		}
		else if (Conv < 0.75)
		{
			rtnInfo.nType = ERROR_BUBBLE;
		}
		else 
		{
			rtnInfo.nType = ERROR_SPOT;
		}
		concat_obj(rtnInfo.regError,regBlob,&rtnInfo.regError);
		return rtnInfo;
	}	

	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	return rtnInfo;
}
//*功能：瓶口轮廓
RtnInfo CCheck::fnFinishCont(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo; 
	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	s_pFinCont pFinCont = para.value<s_pFinCont>();
	s_oFinCont oFinCont = shape.value<s_oFinCont>();
	if (!pFinCont.bEnabled)
	{
		return rtnInfo;
	}

	s_pSideLoc pSideLoc;
	if (m_bLocate && vModelParas[m_nLocateItemID].canConvert<s_pSideLoc>())
	{		
		pSideLoc = vModelParas[m_nLocateItemID].value<s_pSideLoc>();
	}
	if (!m_bLocate || pSideLoc.nMethodIdx != 0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_FIN_CONT;
		rtnInfo.strEx = QObject::tr("The location method must be 'TranslationRotation'");
		return rtnInfo;		
	}

	Hobject objTop,objTopSel,regFinish,regFinishEx,regPart,regSel,regBin;
	Hobject regSelect,regLine,regCont,regTemp,regHori,regMirror,regSymm;
	Hobject regExcludeTemp;
	double dRowRef,dColRef,dAngRef,dThresh;
	Hlong nNum;
	int i;
	HTuple HomMat2DLine,HomMat2DLineInv;


	dRowRef = currentOri.Row;
	dColRef = currentOri.Col;
	dAngRef = currentOri.Angle;
	//gen_rectangle1(&objTop, dRowRef - 20,dColRef - pFinCont.nRegWidth/2, 
	//	dRowRef,dColRef + pFinCont.nRegWidth/2);
	//gen_rectangle1(&regFinishEx, dRowRef  - 30,dColRef - pFinCont.nRegWidth/2,
	//	dRowRef + pFinCont.nRegHeight+10,dColRef + pFinCont.nRegWidth/2);
	//gen_rectangle1(&regFinish, dRowRef  - 30,dColRef - pFinCont.nRegWidth/2,
	//	dRowRef + pFinCont.nRegHeight,dColRef + pFinCont.nRegWidth/2);
	gen_rectangle1(&objTop, dRowRef - 10,dColRef - pFinCont.nRegWidth/2, 
		dRowRef,dColRef + pFinCont.nRegWidth/2);
	gen_rectangle1(&regFinishEx, dRowRef  - 10,dColRef - pFinCont.nRegWidth/2,
		dRowRef + pFinCont.nRegHeight+10,dColRef + pFinCont.nRegWidth/2);
	gen_rectangle1(&regFinish, dRowRef  - 10,dColRef - pFinCont.nRegWidth/2,
		dRowRef + pFinCont.nRegHeight,dColRef + pFinCont.nRegWidth/2);
	//阈值分割得到瓶口区域
	double nPartWidth=pFinCont.nRegWidth/5+4;
	double nPartHeight=pFinCont.nRegHeight+30;
	partition_rectangle(regFinishEx,&regPart,nPartWidth,nPartHeight);
	count_obj(regPart,&nNum);
    /***************获取瓶口*******************/
	for (i=1;i<nNum+1;++i)
	{
		select_obj(regPart,&regSel,i);
		//单独计算每个小区域的分割阈值20130128
		intersection(objTop,regSel,&objTopSel);
		intensity(objTopSel,imgSrc,&dThresh,NULL);
		dThresh-=pFinCont.nSubThresh;
		dThresh = min(255,max(5,dThresh));     //保证dThresh range[5,255]
		reduce_domain(imgSrc,regSel,&regSel);				
		threshold(regSel,&regSel,0,dThresh);
		concat_obj(regSel,regBin,&regBin);
	}
	union1(regBin,&regBin);
	connection(regBin,&regBin);
	select_shape(regBin,&regSelect,"area","and",100,99999999);
	count_obj(regSelect,&nNum);
	if (nNum==0)
	{	
		concat_obj(rtnInfo.regError,regBin,&rtnInfo.regError);
		rtnInfo.nType = ERROR_FIN_CONT;
		rtnInfo.strEx = QObject::tr("Height of the Detection region is too small, or the bottle edge is not clear.");
		return rtnInfo;
	}
	union1(regSelect,&regSelect);
	closing_rectangle1(regSelect,&regSelect,1,nPartHeight);
	connection(regSelect,&regSelect);
	select_shape(regSelect,&regSelect,"width","and",pFinCont.nRegWidth/2,pFinCont.nRegWidth*1.5);
	count_obj(regSelect,&nNum);
	if (nNum==0)
	{
		concat_obj(rtnInfo.regError,regSelect,&rtnInfo.regError);
		rtnInfo.nType = ERROR_FIN_CONT;
		rtnInfo.strEx = QObject::tr("Please Check whether the contour of the bottle mouth is disconnected due to non-uniform gray");
		return rtnInfo;
	}
	gen_region_line(&regLine,dRowRef + pFinCont.nRegHeight,dColRef - pFinCont.nRegWidth/2,
		dRowRef + pFinCont.nRegHeight,dColRef + pFinCont.nRegWidth/2);
	union2(regSelect,regLine,&regSelect);
	fill_up(regSelect,&regSelect);
	opening_rectangle1(regSelect,&regSelect,1,2);
	//生成显示用的轮廓线和检测区域
	boundary(regSelect,&regCont,"outer");
	erosion_rectangle1(regFinish,&regTemp,2,2);
	intersection(regCont,regTemp,&regCont);
	copy_obj(regFinish,&oFinCont.oCheckRegion,1,-1);
	copy_obj(regCont,&oFinCont.oFinishCont,1,-1);
	shape.setValue(oFinCont);
	//计算对称性差异
	vector_angle_to_rigid(dRowRef,dColRef,PI/2,dRowRef,dColRef,dAngRef,&HomMat2DLine);
	hom_mat2d_invert(HomMat2DLine, &HomMat2DLineInv);
	affine_trans_region(regSelect, &regHori,HomMat2DLineInv, "false");
	intersection(regHori,regFinish,&regHori);
	double offCol = m_nWidth/2 - dColRef;
	move_region(regHori, &regHori, HTuple(0), HTuple(offCol));
	mirror_region(regHori, &regMirror, "column", m_nWidth); 
	symm_difference(regMirror, regHori, &regSymm);
	//判断缺陷
	erosion_rectangle1(regSymm, &regExcludeTemp, pFinCont.nGapWidth,pFinCont.nGapHeight);//缺口高度*宽度大小的不认为有缺陷
	//ExtExcludeAffinedDefect(rtnInfo,regSymm,regExcludeTemp,-offCol,HomMat2DLine,EXCLUDE_CAUSE_WIDTH_AND_HEIGHT,EXCLUDE_TYPE_GENERALDEFEXTS,pSideLoc.strName,true);
	connection(regExcludeTemp, &regSymm);
	select_shape(regSymm, &regSymm, "area", "and",3, 99999999);
	count_obj(regSymm, &nNum);
	if (nNum > 0)
	{
		dilation_rectangle1(regSymm, &regExcludeTemp, pFinCont.nGapWidth, pFinCont.nGapHeight);
		select_shape(regExcludeTemp,&regSymm,"area","and",pFinCont.nArea,999999);
		ExtExcludeAffinedDefect(rtnInfo,regExcludeTemp,regSymm,-offCol,HomMat2DLine,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pSideLoc.strName,true);
		count_obj(regSymm,&nNum);
		if (nNum>0)
		{
			move_region(regSymm, &regSymm, 0, -offCol);
			affine_trans_region(regSymm, &regSymm, HomMat2DLine, "false");	
			concat_obj(regSymm,rtnInfo.regError,&rtnInfo.regError);
			rtnInfo.nType = ERROR_FIN_CONT;
			return rtnInfo;
		}
	}

	// 计算左中右三条垂直线与瓶口轮廓的交点的上下关系，
	// 合格的瓶子，左=右<=中
	Hobject LeftLn, RightLn, MidLn, LeftLn2, RightLn2;
	Hobject LeftPt, RightPt, MidPt, LeftPt2, RightPt2;
	gen_region_line(&MidLn, dRowRef-20, m_nWidth/2, dRowRef + pFinCont.nRegHeight, m_nWidth/2);
	gen_region_line(&LeftLn,dRowRef-20, m_nWidth/2-pFinCont.nRegWidth/2,
		dRowRef + pFinCont.nRegHeight, m_nWidth/2-pFinCont.nRegWidth/2);
	gen_region_line(&RightLn, dRowRef-20, m_nWidth/2+pFinCont.nRegWidth/2, 
		dRowRef + pFinCont.nRegHeight, m_nWidth/2+pFinCont.nRegWidth/2);//中左右
	gen_region_line(&LeftLn2, dRowRef-20, m_nWidth/2-pFinCont.nRegWidth/4,
		dRowRef + pFinCont.nRegHeight, m_nWidth/2-pFinCont.nRegWidth/4);
	gen_region_line(&RightLn2, dRowRef-20, m_nWidth/2+pFinCont.nRegWidth/4, 
		dRowRef + pFinCont.nRegHeight, m_nWidth/2+pFinCont.nRegWidth/4);

	intersection(MidLn, regHori, &MidPt);
	intersection(LeftLn, regHori, &LeftPt);
	intersection(RightLn, regHori, &RightPt);
	intersection(LeftLn2, regHori, &LeftPt2);
	intersection(RightLn2, regHori, &RightPt2);

	HTuple RowPts, ColPts;
	double MidRow, LeftRow, RightRow, LeftRow2, RightRow2;
	get_region_points(MidPt, &RowPts, &ColPts);
	if (RowPts.Num() == 0)
	{
		MidRow = dRowRef;
	}
	else
	{
		MidRow = RowPts[0];
	}
	get_region_points(LeftPt, &RowPts, &ColPts);
	if (RowPts.Num() == 0)
	{
		LeftRow = dRowRef;
	}
	else
	{
		LeftRow = RowPts[0];
	}
	get_region_points(RightPt, &RowPts, &ColPts);
	if (RowPts.Num() == 0)
	{
		RightRow = dRowRef;
	}
	else
	{
		RightRow = RowPts[0];
	}
	get_region_points(LeftPt2, &RowPts, &ColPts);
	if (RowPts.Num() == 0)
	{
		LeftRow2 = dRowRef;
	}
	else
	{
		LeftRow2 = RowPts[0];
	}
	get_region_points(RightPt2, &RowPts, &ColPts);
	if (RowPts.Num() == 0)
	{
		RightRow2 = dRowRef;
	}
	else
	{
		RightRow2 = RowPts[0];
	}

	int nDiff = pFinCont.nNotchHeight;
	//判断中心凹口
	if (MidRow > LeftRow+nDiff
		|| MidRow > RightRow+nDiff
		|| MidRow > LeftRow2+nDiff
		|| MidRow > RightRow2+nDiff)
	{
		Hobject MouthGap;
		gen_circle(&MouthGap, dRowRef, dColRef, 13.5);
		concat_obj(rtnInfo.regError,MouthGap,&rtnInfo.regError);
		rtnInfo.strEx = QObject::tr("Center notch defects");
		rtnInfo.nType = ERROR_FIN_CONT;
		return rtnInfo;
	}

	//2018.3-增加双口缺陷判断（利用拐点信息）
	if (pFinCont.bCheckOverPress)
	{
		Hobject oRect,ImgReduced,Edges,SelXLD,UnionConts,SortEdges;
		Hobject TopEdge,tempCircle;
		HTuple Row,Column,ContRow1,ContCol1,ContRow2,ContCol2;
		double DistMin,distToLeft,distToRight;

		//选取瓶口边缘
		gen_rectangle1(&oRect, dRowRef-10, dColRef-pFinCont.nRegWidth/2, 
			dRowRef+pFinCont.nRegHeight, dColRef+pFinCont.nRegWidth/2);
		reduce_domain(imgSrc, oRect, &ImgReduced);
		edges_sub_pix (ImgReduced, &Edges, "canny", 2, 20, 40);
		select_shape_xld (Edges, &SelXLD, "contlength", "and", 20, 99999); 
		union_adjacent_contours_xld (SelXLD, &UnionConts, 1, 1, "attr_keep"); //MaxDistAbs不能设太大，不然会有误检
		sort_contours_xld (UnionConts, &SortEdges,"upper_left", "true", "row");
		count_obj (SortEdges, &nNum);
		if (nNum <1)
		{
			return rtnInfo;
		}

		//查找拐点位置    
		points_harris (ImgReduced, 1, 4, 0.04, 10000, &Row, &Column);
		//gen_region_points (Points, Row, Column) //只用于测试显示
		tuple_length (Row, &nNum);
		if (nNum < 1)
		{
			return rtnInfo;
		}

		// 筛选拐点-离边缘最近&&与边缘最左及最右有一定距离
		select_obj (SortEdges, &TopEdge, 1);
		smallest_rectangle1_xld (TopEdge, &ContRow1, &ContCol1, &ContRow2, &ContCol2);
		//gen_empty_obj (DefectPoints)
		for (int index=0; index<=nNum-1; index++)
		{
			distance_pc (TopEdge, Row[index].D(), Column[index].D(), &DistMin, NULL);
			distToLeft = abs(Column[index].D()-ContCol1[0].D());
			distToRight = abs(Column[index].D()-ContCol2[0].D());
			if ( (DistMin<1.5) && (distToLeft>10) && (distToRight>10) )
			{
				gen_circle (&tempCircle, Row[index], Column[index], 5);
				concat_obj (rtnInfo.regError, tempCircle, &rtnInfo.regError);
			}
		}

		count_obj (rtnInfo.regError, &nNum);
		if (nNum > 0)
		{
			rtnInfo.nType = ERROR_OVERPRESS;
			return rtnInfo;
		}
	}

	return rtnInfo;
}
//功能：搜索轮廓
int CCheck::SearchContourPoints(Hobject &imgSrc,Hobject &regRect,int nThresh,vector<HTuple> *contPoints)
{
	HTuple rPtTemp,cPtTemp,rowPtsL,colPtsL,rowPtsR,colPtsR;
	Hlong row1,row2,col1,col2;
	Hlong colBotL=0,colBotR=0,colTopL=0,colTopR=0;
	Hlong rowBotL,rowBotR,rowTopL,rowTopR;
	Hobject lineBot,RegL,RegR;
	bool openL=false,openR=false;
	int nCount;
	int startIdx,endIdx;
	int i;
	int nRet;

	rowPtsL=HTuple();
	colPtsL=HTuple();
	rowPtsR=HTuple();
	colPtsR=HTuple();
	smallest_rectangle1(regRect,&row1,&col1,&row2,&col2);	

	int height=row2-row1;
	int width=col2-col1;
	if (height<6 || width<6)
		return -1;//-1:高度或宽度太小

	//判断左右是否都检测
	gen_region_line(&lineBot,row2,col1,row2,col2);
	nRet=findEdgePointSingle(imgSrc,lineBot,&rPtTemp,&cPtTemp,10,L2R);
	if (nRet==1)
	{	
		openL=true;
		colBotL=cPtTemp[0];	
		rowBotL=rPtTemp[0];
	}
	nRet=findEdgePointSingle(imgSrc,lineBot,&rPtTemp,&cPtTemp,10,R2L);
	if (nRet==1)
	{	
		openR=true;
		colBotR=cPtTemp[0];
		rowBotR=rPtTemp[0];
	}
	if (!openL || !openR)
		return -2;//-2:检测区域左侧或右侧未找到边界

	//*****20120505增加连瓶判断*****//	
	Hobject rectTemp,domainTemp,sobelTemp;
	Hlong numTemp,colTemp;	
	//左边连瓶
	gen_rectangle1(&rectTemp,row1,col1-10,rowBotL,colBotL+10);//左移10,防止很细的连瓶边界漏	
	reduce_domain(imgSrc,rectTemp,&domainTemp);
	sobel_amp(domainTemp,&sobelTemp,"sum_abs",3);
	threshold(sobelTemp,&sobelTemp,nThresh,255);	
	connection(sobelTemp,&sobelTemp);
	closing_rectangle1(sobelTemp,&sobelTemp,3,8);		
	select_shape(sobelTemp,&sobelTemp,HTuple("height").Concat("area"),"and",HTuple((row2-row1)/4+3).Concat(20),HTuple(99999).Concat(99999));	
	count_obj(sobelTemp,&numTemp);
	if (numTemp>1)
	{
		sort_region(sobelTemp,&sobelTemp,"upper_right","false","column");
		select_obj(sobelTemp,&sobelTemp,2);			
		smallest_rectangle1(sobelTemp,NULL,NULL,NULL,&colTemp);
		if (abs(colTemp-colBotL)>10)//防止把真实异形截掉
			col1=colTemp+1;
	}
	//右边连瓶
	gen_rectangle1(&rectTemp,row1,colBotR-10,row2,col2+10);//右移10,防止很细的连瓶边界漏
	reduce_domain(imgSrc,rectTemp,&domainTemp);
	sobel_amp(domainTemp,&sobelTemp,"sum_abs",3);
	threshold(sobelTemp,&sobelTemp,nThresh,255);	
	connection(sobelTemp,&sobelTemp);
	closing_rectangle1(sobelTemp,&sobelTemp,3,8);		
	select_shape(sobelTemp,&sobelTemp,HTuple("height").Concat("area"),"and",HTuple((row2-row1)/4+3).Concat(20),HTuple(99999).Concat(99999));	
	count_obj(sobelTemp,&numTemp);
	if (numTemp>1)
	{
		sort_region(sobelTemp,&sobelTemp,"upper_left","true","column");
		select_obj(sobelTemp,&sobelTemp,2);				
		smallest_rectangle1(sobelTemp,NULL,&colTemp,NULL,NULL);
		if (abs(colTemp-colBotR)>10)//防止把真实异形截掉
			col2=colTemp-1;
	}		


	//计算左轮廓
	if (openL)
	{
		Hobject imageSobel,edgeRegs;
		HTuple absHisto,relHisto,minThresh,maxThresh;
		Hlong regNum;
		nCount=0;

		//生成区域
		double colRight;	
		gen_region_line(&lineBot,row1,col1,row1,col2);
		nRet=findEdgePointSingle(imgSrc,lineBot,&rPtTemp,&cPtTemp,10,L2R);
		if (nRet==1)
		{
			colTopL=cPtTemp[0];
			rowTopL=rPtTemp[0];
		}
		else
		{
			colTopL=colBotL;
			rowTopL=row1;
		}
		colRight=colTopL+50;	
		rPtTemp=HTuple();
		cPtTemp=HTuple();
		gen_rectangle1(&RegL,row1,col1,row2,colRight);	
		//分块
		int nRegionWidth = colRight-col1+1;
		int nRegionHeight = row2-row1;
		int nPartWidth = nRegionWidth;
		int nPartHeight = nRegionHeight/*/2 + 1*/;
		Hobject PartionRegion;			
		partition_rectangle(RegL, &PartionRegion, nPartWidth, nPartHeight);		
		connection(PartionRegion, &PartionRegion);		
		Hobject PartSel, tempRegion;
		Hobject ImgPart;			
		Hlong nNumber;
		select_shape(PartionRegion, &PartionRegion, "area", "and", 100, 99999999);
		count_obj(PartionRegion, &nNumber);
		for (i = 0; i< nNumber; ++i)
		{
			select_obj(PartionRegion, &PartSel, i+1); 		
			reduce_domain(imgSrc, PartSel, &ImgPart);	
			sobel_amp(ImgPart,&imageSobel,"sum_abs",3);
			threshold(imageSobel,&tempRegion,nThresh,255);	
			connection(tempRegion,&tempRegion);
			closing_rectangle1(tempRegion,&tempRegion,3,8);//连接边缘，太大会漏报0208			
			select_shape(tempRegion,&tempRegion,HTuple("height"),"and",HTuple(nPartHeight/2),HTuple(99999));
			Hlong edgeNums;
			count_obj(tempRegion,&edgeNums);
			if (edgeNums==0)
				continue;
			HTuple edgeCol;
			area_center(tempRegion,NULL,NULL,&edgeCol);
			int minCol=9999,edgeIdx=0;
			for (i=0;i<edgeNums;i++)
			{
				if (edgeCol[i].I()<minCol)
				{
					minCol=edgeCol[i].I();
					edgeIdx=i;
				}
			}
			select_obj(tempRegion,&tempRegion,edgeIdx+1);
			concat_obj(edgeRegs, tempRegion, &edgeRegs);
		}
		union1(edgeRegs, &edgeRegs);
		Hlong edgeArea;
		area_center(edgeRegs,&edgeArea,NULL,NULL);
		if (edgeArea<5)
			return -3;//-3:搜索不到边缘

		connection(edgeRegs,&edgeRegs);	
		count_obj(edgeRegs,&regNum);	
		//边缘不连续时
		if (regNum>1)
		{
			HTuple rPtSingle,cPtSingle;
			Hobject edgeSingle;		
			sort_region(edgeRegs,&edgeRegs,"upper_left","true","row");

			for (i=0;i<regNum;i++)
			{
				select_obj(edgeRegs,&edgeSingle,i+1);
				get_region_points(edgeSingle,&rPtSingle,&cPtSingle);
				tuple_concat(rPtTemp,rPtSingle,&rPtTemp);
				tuple_concat(cPtTemp,cPtSingle,&cPtTemp);
			}
		}
		else
			get_region_points(edgeRegs,&rPtTemp,&cPtTemp);

		int lastRow=rPtTemp[0].L()-1;		
		for (i=0;i<rPtTemp.Num();i++)
		{			
			if ((rPtTemp[i].L()-lastRow)>0)
			{
				lastRow=rPtTemp[i].L();
				rowPtsL.Append(rPtTemp[i]);
				colPtsL.Append(cPtTemp[i]);	
			}				
		}
		nCount=rowPtsL.Num();
		if (nCount>5)
		{ 
			//取消轮廓线预处理
			startIdx=0;
			endIdx=nCount-1;				
			tuple_select_range(rowPtsL,startIdx,endIdx,&rowPtsL);
			tuple_select_range(colPtsL,startIdx,endIdx,&colPtsL);	
			contPoints->push_back(rowPtsL);
			contPoints->push_back(colPtsL);
		}
		else 
			return -3;
	}	
	//计算右轮廓
	if (openR)
	{
		Hobject imageSobel,edgeRegs;
		HTuple absHisto,relHisto,minThresh,maxThresh;
		Hlong regNum;
		nCount=0;

		//生成区域
		double colRight;
		gen_region_line(&lineBot,row1,col1,row1,col2);
		nRet=findEdgePointSingle(imgSrc,lineBot,&rPtTemp,&cPtTemp,10,R2L);
		if (nRet==1)
		{
			colTopR=cPtTemp[0];
			rowTopR=rPtTemp[0];
		}
		else
		{
			colTopR=colBotR;
			rowTopR=row2;
		}
		colRight=colTopR-50;			
		rPtTemp=HTuple();
		cPtTemp=HTuple();		
		gen_rectangle1(&RegR,row1,colRight,row2,col2);				
		//分块
		int nRegionWidth = col2-colRight+1;
		int nRegionHeight = row2-row1;
		int nPartWidth = nRegionWidth;
		int nPartHeight = nRegionHeight/*/2 + 1*/;
		Hobject PartionRegion;			
		partition_rectangle(RegR, &PartionRegion, nPartWidth, nPartHeight);		
		connection(PartionRegion, &PartionRegion);		
		Hobject PartSel, tempRegion;
		Hobject ImgPart;
		Hlong nNumber;
		select_shape(PartionRegion, &PartionRegion, "area", "and", 100, 99999999);
		count_obj(PartionRegion, &nNumber);		
		for (i = 0; i< nNumber; ++i)
		{
			select_obj(PartionRegion, &PartSel, i+1);		
			reduce_domain(imgSrc, PartSel, &ImgPart);	
			sobel_amp(ImgPart,&imageSobel,"sum_abs",3);
			threshold(imageSobel,&tempRegion,nThresh,255);		
			connection(tempRegion,&tempRegion);
			closing_rectangle1(tempRegion,&tempRegion,3,8);
			select_shape(tempRegion,&tempRegion,HTuple("height"),"and",HTuple(nPartHeight/2),HTuple(99999));
			Hlong edgeNums;
			count_obj(tempRegion,&edgeNums);
			if (edgeNums==0)
				continue;	
			HTuple edgeCol;
			area_center(tempRegion,NULL,NULL,&edgeCol);
			int maxCol=0,edgeIdx=0;
			for (i=0;i<edgeNums;i++)
			{
				if (edgeCol[i].I()>maxCol)
				{
					maxCol=edgeCol[i].I();
					edgeIdx=i;
				}
			}		
			select_obj(tempRegion,&tempRegion,edgeIdx+1);//取最右边的
			concat_obj(edgeRegs, tempRegion, &edgeRegs);	
		}	
		union1(edgeRegs, &edgeRegs);
		Hlong edgeArea;
		area_center(edgeRegs,&edgeArea,NULL,NULL);
		if (edgeArea<5)
			return -3;

		connection(edgeRegs,&edgeRegs);	
		count_obj(edgeRegs,&regNum);	
		//边缘不连续时
		if (regNum>1)
		{
			HTuple rPtSingle,cPtSingle;
			Hobject edgeSingle;
			sort_region(edgeRegs,&edgeRegs,"upper_left","true","row");			

			for (i=0;i<regNum;i++)
			{
				select_obj(edgeRegs,&edgeSingle,i+1);
				get_region_points(edgeSingle,&rPtSingle,&cPtSingle);
				tuple_concat(rPtTemp,rPtSingle,&rPtTemp);
				tuple_concat(cPtTemp,cPtSingle,&cPtTemp);
			}			
		}
		else
			get_region_points(edgeRegs,&rPtTemp,&cPtTemp);	

		int lastRow=rPtTemp[0].L(),lastCol=cPtTemp[0].L()-1;
		for (i=0;i<rPtTemp.Num();i++)
		{//注意与左边的区别：取区域右边缘
			if ((rPtTemp[i].L()-lastRow)>=0)
			{
				if ((rPtTemp[i].L()-lastRow)>0)
				{
					rowPtsR.Append(lastRow);
					colPtsR.Append(lastCol);	
				}
				lastRow=rPtTemp[i].L();
				lastCol=cPtTemp[i].L();
			}				
		}
		nCount=rowPtsR.Num();

		if (nCount>5)
		{
			//取消轮廓预处理
			startIdx=0;
			endIdx=nCount-1;	
			tuple_select_range(rowPtsR,startIdx,endIdx,&rowPtsR);
			tuple_select_range(colPtsR,startIdx,endIdx,&colPtsR);	
			contPoints->push_back(rowPtsR);
			contPoints->push_back(colPtsR);
		}
	}
	//计算对称的中心点和角度，用于左右平移时也能检测瓶身轮廓
	m_nColBodyCont = 0;
	m_dAngBodyCont = 0;
	if (m_bLocate && vModelParas[m_nLocateItemID].canConvert<s_pSideLoc>())
	{
		s_pSideLoc pSideLoc = vModelParas[m_nLocateItemID].value<s_pSideLoc>();
		if ((colBotL!=0&&colBotR!=0&&colTopL!=0&&colTopR!=0)&&
			pSideLoc.nMethodIdx == 1)
		{
			m_nColBodyCont = (colBotL+colBotR+colTopL+colTopR)/4;
			double ptRowTop=(rowTopL+rowTopR)/2;
			double ptColTop=(colTopL+colTopR)/2;
			double ptRowBot=(rowBotL+rowBotR)/2;
			double ptColBot=(colBotL+colBotR)/2;
			line_orientation(ptRowTop,ptColTop,ptRowBot,ptColBot,&m_dAngBodyCont);		
		}
	}
	return 1;
}
//*功能：瓶肩轮廓
RtnInfo CCheck::fnNeckCont(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pNeckCont pNeckCont = para.value<s_pNeckCont>();
	s_oNeckCont oNeckCont = shape.value<s_oNeckCont>();
	if (!pNeckCont.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	gen_empty_obj(&oNeckCont.oNeckCont);
	gen_empty_obj(&oNeckCont.oNeckLine);
	oNeckCont.nNeckHei = 0;
	//检查定位方法
	s_pSideLoc pSideLoc;
	bool bLocPass = false;
	if (m_bLocate && vModelParas[m_nLocateItemID].canConvert<s_pSideLoc>())
	{
		pSideLoc = vModelParas[m_nLocateItemID].value<s_pSideLoc>();
		if (pSideLoc.nMethodIdx == 0)
		{
			bLocPass = true;
		}
	}
	if (!bLocPass)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_NECK_CONT;
		rtnInfo.strEx = QObject::tr("The location method must be 'TranslationRotation'");
		return rtnInfo;	
	}
	//搜索边缘
	int nNeckHeight,nNeckWidth,nNeckWidthTemp,i;
	vector<HTuple > contPoints;
	Hobject curvSegL,curvSegR,regExcludeTemp;
	//2014.9.12 区域变换时，若靠边会丢失变小，导致轮廓尺寸区域连续误报，均修改为轮廓
	Hobject hCheckRegion;
	double rectRow1,rectCol1,rectRow2,rectCol2;
	smallest_rectangle1_xld(oNeckCont.oCheckRegion,&rectRow1,&rectCol1,&rectRow2,&rectCol2);
	gen_rectangle1(&hCheckRegion,rectRow1,rectCol1,rectRow2,rectCol2);
	int rectHeight=rectRow2-rectRow1;
	int nRet=SearchContourPoints(imgSrc,hCheckRegion,pNeckCont.nThresh,&contPoints);
	if (nRet < 0)
	{
		switch(nRet)
		{
		case  -1:
			rtnInfo.strEx = QObject::tr("Width or height of detection region is too small");
			break;
		case -2:
			rtnInfo.strEx = QObject::tr("Can not search the contour");
			break;
		case -3:
			rtnInfo.strEx = QObject::tr("Can not search the contour");
			break;
		default:
			break;
		}
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_NECK_CONT;		
		return rtnInfo;
	}
	//计算脖高
	if (nRet>0 && contPoints.size() > 3)
	{
		gen_contour_polygon_xld(&curvSegL,contPoints[0],contPoints[1]);
		gen_contour_polygon_xld(&curvSegR,contPoints[2],contPoints[3]);	
		double fLow = pNeckCont.nNeckHeiL;
		double fHigh = pNeckCont.nNeckHeiH;
		int numL=contPoints[0].Num();
		int numR=contPoints[2].Num();
		if (numL>rectHeight/2 && numR>rectHeight/2)
		{
			s_oSideLoc oSideLoc = vModelShapes[m_nLocateItemID].value<s_oSideLoc>();
			nNeckWidth=oSideLoc.ori.nCol12-oSideLoc.ori.nCol11;
			int botNum=numL>numR?numR:numL;
			for (i=botNum-1;i>1;i--)//2014.5.22 nanjc 修改i>0  => i>1 ，循环完成后可能导致i=-1，越界异常
			{
				nNeckWidthTemp=contPoints[3][i].L()-contPoints[1][i].L();
				if (abs(nNeckWidth-nNeckWidthTemp)>10)
					break;
				i-=1;//隔一个计算一次
			}
			nNeckHeight=contPoints[0][i].L()-currentOri.Row;
			oNeckCont.nNeckHei = nNeckHeight;
			Hobject neckLine;
			gen_region_line(&neckLine,contPoints[0][i].L(),contPoints[1][i].L(),contPoints[2][i].L(),contPoints[3][i].L());
			concat_obj(neckLine,oNeckCont.oNeckCont,&oNeckCont.oNeckLine);
		}
		else
		{
			gen_rectangle1(&rtnInfo.regError,120,20,220,120);
			rtnInfo.nType = ERROR_NECK_CONT;
			rtnInfo.strEx = QObject::tr("Height of the contour is smaller than half of the actual region");
			return rtnInfo;
		}
		gen_region_polygon(&curvSegL,contPoints[0],contPoints[1]);			
		gen_region_polygon(&curvSegR,contPoints[2],contPoints[3]);
		concat_obj(oNeckCont.oNeckCont,curvSegL,&oNeckCont.oNeckCont);
		concat_obj(oNeckCont.oNeckCont,curvSegR,&oNeckCont.oNeckCont);
		shape.setValue(oNeckCont);

		if (nNeckHeight< fLow || nNeckHeight > fHigh)
		{
			concat_obj(rtnInfo.regError,oNeckCont.oNeckLine,&rtnInfo.regError);
			rtnInfo.nType = ERROR_NECK_CONT;
			return rtnInfo;
		}
		//判断对称性
		if (numR>5&& numL>5)
		{
			Hobject regCurvSegL,regCurvSegR;
			HTuple HomMat2DLine,HomMat2DLineInv;
			vector_angle_to_rigid(currentOri.Row,currentOri.Col,PI/2,currentOri.Row,
				currentOri.Col,currentOri.Angle,&HomMat2DLine);
			hom_mat2d_invert(HomMat2DLine, &HomMat2DLineInv);

			gen_region_polygon(&regCurvSegL,contPoints[0],contPoints[1]);
			gen_region_polygon(&regCurvSegR,contPoints[2],contPoints[3]);
			Hobject line1,line2,line3,line4;
			double b1,b2,b3,b4,k;
			float angle=currentOri.Angle-PI/2;
			Hobject totalRegion,horRegion,mirrorRegion,diffRegion;
			Hlong nNum;

			int numL=contPoints[0].Num();
			int numR=contPoints[2].Num();
			k=tan(angle);	
			b1=contPoints[1][0].D()-k*contPoints[0][0].D();
			b2=contPoints[1][numL-1].D()-k*contPoints[0][numL-1].D();
			b3=contPoints[3][0].D()-k*contPoints[2][0].D();
			b4=contPoints[3][numR-1].D()-k*contPoints[2][numR-1].D();

			gen_region_line(&line1,contPoints[0][0].D(),contPoints[1][0].D(),contPoints[0][0].D()-m_nWidth*sin(angle),contPoints[1][0].D()+m_nWidth*cos(angle));
			gen_region_line(&line2,contPoints[0][numL-1].D(),contPoints[1][numL-1].D(),contPoints[0][numL-1].D()-m_nWidth*sin(angle),contPoints[1][numL-1].D()+m_nWidth*cos(angle));
			gen_region_line(&line3,contPoints[2][0].D(),contPoints[3][0].D(),contPoints[2][0].D()+m_nWidth*sin(angle),contPoints[3][0].D()-m_nWidth*cos(angle));
			gen_region_line(&line4,contPoints[2][numR-1].D(),contPoints[3][numR-1].D(),contPoints[2][numR-1].D()+m_nWidth*sin(angle),contPoints[3][numR-1].D()-m_nWidth*cos(angle));

			concat_obj(totalRegion,regCurvSegL,&totalRegion);
			concat_obj(totalRegion,regCurvSegR,&totalRegion);
			concat_obj(totalRegion,line1,&totalRegion);
			concat_obj(totalRegion,line2,&totalRegion);
			concat_obj(totalRegion,line3,&totalRegion);
			concat_obj(totalRegion,line4,&totalRegion);

			union1(totalRegion,&totalRegion);					
			fill_up(totalRegion,&totalRegion);					
			opening_circle(totalRegion,&totalRegion,1);							
			affine_trans_region(totalRegion, &horRegion,HomMat2DLineInv, "false");
			double meanCol;
			area_center(horRegion,NULL,NULL,&meanCol);
			double offCol = m_nWidth/2 - meanCol;
			move_region(horRegion, &horRegion, HTuple(0), HTuple(offCol));				
			mirror_region(horRegion, &mirrorRegion, "column", m_nWidth); 
			symm_difference(mirrorRegion, horRegion, &diffRegion);	
			opening_circle(diffRegion,&diffRegion,2.5);
			connection(diffRegion, &regExcludeTemp);
			select_shape(regExcludeTemp, &diffRegion, "area", "and", pNeckCont.nArea, 99999999);
			ExtExcludeAffinedDefect(rtnInfo,regExcludeTemp,diffRegion,-offCol,HomMat2DLine,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pNeckCont.strName,true);
			count_obj(diffRegion, &nNum);
			if (nNum > 0)
			{	
				move_region(diffRegion, &diffRegion, 0, -offCol);
				affine_trans_region(diffRegion, &diffRegion, HomMat2DLine, "false");	
				concat_obj(rtnInfo.regError,diffRegion,&rtnInfo.regError);
				rtnInfo.nType = ERROR_NECK_CONT;
				return rtnInfo;
			}
		}
	}	

	return rtnInfo;
}
//*功能：瓶身轮廓
RtnInfo CCheck::fnBodyCont(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pBodyCont pBodyCont = para.value<s_pBodyCont>();
	s_oBodyCont oBodyCont = shape.value<s_oBodyCont>();
	if (!pBodyCont.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}
	
	gen_empty_obj(&oBodyCont.oBodyCont);
	oBodyCont.nBodyWidth = 0;
	//检查定位方法
	s_pSideLoc pSideLoc;
	s_oSideLoc oSideLoc;
	bool bLocPass = false;
	if (m_bLocate && vModelParas[m_nLocateItemID].canConvert<s_pSideLoc>())
	{
		pSideLoc = vModelParas[m_nLocateItemID].value<s_pSideLoc>();
		oSideLoc = vModelShapes[m_nLocateItemID].value<s_oSideLoc>();
		if (pSideLoc.nMethodIdx == 0 || pSideLoc.nMethodIdx ==1)
		{
			bLocPass = true;//允许平移旋转或横向平移两种方法
		}
	}
	if (!bLocPass)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_BODY_CONT;
		rtnInfo.strEx = QObject::tr("The location method must be 'TranslationRotation' or 'HorizontalTranslation'");
		return rtnInfo;	
	}
	//2014.9.12 区域变换时，若靠边会丢失变小，导致轮廓尺寸区域连续误报，均修改为轮廓
	Hobject hCheckRegion;
	double dCheckRow1,dCheckCol1,dCheckRow2,dCheckCol2;
	smallest_rectangle1_xld(oBodyCont.oCheckRegion,&dCheckRow1,&dCheckCol1,&dCheckRow2,&dCheckCol2);
	gen_rectangle1(&hCheckRegion,dCheckRow1,dCheckCol1,dCheckRow2,dCheckCol2);
	vector<HTuple > contPoints;
	Hobject lineSegL,lineSegR,regExcludeTemp;
	int nRet=SearchContourPoints(imgSrc,hCheckRegion,pBodyCont.nThresh,&contPoints);
	if (nRet < 0)
	{
		switch(nRet)
		{
		case  -1:
			rtnInfo.strEx = QObject::tr("Width or height of detection region is too small");
			break;
		case -2:
			rtnInfo.strEx = QObject::tr("Can not search the contour");
			break;
		case -3:
			rtnInfo.strEx = QObject::tr("Can not search the contour");
			break;
		default:
			break;
		}
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_BODY_CONT;		
		return rtnInfo;
	}	
	//计算瓶身轮廓宽度
	if (nRet>0 && contPoints.size() > 3)
	{
		int numL=contPoints[0].Num();
		int numR=contPoints[2].Num();
		double distMaxL=0,distMaxR=0,distMin;
		if (numL>10)
		{
			gen_region_polygon(&lineSegL,contPoints[0],contPoints[1]);	
			concat_obj(oBodyCont.oBodyCont,lineSegL,&oBodyCont.oBodyCont);
			distance_lr(lineSegL,contPoints[0][0].L(),contPoints[1][0].L(),contPoints[0][numL-1].L(),contPoints[1][numL-1].L(),&distMin,&distMaxL);
			oBodyCont.nBodyWidth=distMaxL;
			shape.setValue(oBodyCont);
			if (distMaxL>=pBodyCont.nWidth)
			{				
				concat_obj(rtnInfo.regError,lineSegL,&rtnInfo.regError);
				rtnInfo.nType = ERROR_BODY_CONT;
				return rtnInfo;
			}
		}
		if (numR>10)
		{
			gen_region_polygon(&lineSegR,contPoints[2],contPoints[3]);		
			concat_obj(oBodyCont.oBodyCont,lineSegR,&oBodyCont.oBodyCont);
			distance_lr(lineSegR,contPoints[2][0].L(),contPoints[3][0].L(),contPoints[2][numR-1].L(),contPoints[3][numR-1].L(),&distMin,&distMaxR);
			oBodyCont.nBodyWidth=distMaxL;
			shape.setValue(oBodyCont);
			if (distMaxR>=pBodyCont.nWidth)
			{						
				concat_obj(rtnInfo.regError,lineSegR,&rtnInfo.regError);
				rtnInfo.nType = ERROR_BODY_CONT;
				return rtnInfo;
			}

		}
		oBodyCont.nBodyWidth=distMaxL>distMaxR?distMaxL:distMaxR;
		shape.setValue(oBodyCont);
		//判断对称性
		if (numR>10 && numL>10)
		{
			Hlong row1,row2,col1,col2;
			Hobject line1,line2,line3,line4,regRect;
			double b1,b2,b3,b4,k;
			float angle;
			HTuple HomMat2DLine,HomMat2DLineInv;
			vector_angle_to_rigid(currentOri.Row,currentOri.Col,PI/2,currentOri.Row,
				currentOri.Col,currentOri.Angle,&HomMat2DLine);
			hom_mat2d_invert(HomMat2DLine, &HomMat2DLineInv);

			//if (pSideLoc.nMethodIdx == 0)
			if (m_nColBodyCont == 0)	//2014.6.11 nanjc 与C++一致，暂不清楚修改原因
			{
				m_nColBodyCont=currentOri.Col;
				m_dAngBodyCont=currentOri.Angle;
			}
			if (m_dAngBodyCont>0)
			{
				angle=m_dAngBodyCont-PI/2;
			}
			else
				angle=m_dAngBodyCont+PI/2;
			Hobject totalRegion,horRegion,mirrorRegion,diffRegion;
			Hlong nNum;

			int numL=contPoints[0].Num();
			int numR=contPoints[2].Num();
			k=tan(angle);	
			b1=contPoints[1][0].D()-k*contPoints[0][0].D();
			b2=contPoints[1][numL-1].D()-k*contPoints[0][numL-1].D();
			b3=contPoints[3][0].D()-k*contPoints[2][0].D();
			b4=contPoints[3][numR-1].D()-k*contPoints[2][numR-1].D();

			gen_region_line(&line1,contPoints[0][0].D(),contPoints[1][0].D(),contPoints[0][0].D()-m_nWidth*sin(angle),contPoints[1][0].D()+m_nWidth*cos(angle));
			gen_region_line(&line2,contPoints[0][numL-1].D(),contPoints[1][numL-1].D(),contPoints[0][numL-1].D()-m_nWidth*sin(angle),contPoints[1][numL-1].D()+m_nWidth*cos(angle));
			gen_region_line(&line3,contPoints[2][0].D(),contPoints[3][0].D(),contPoints[2][0].D()+m_nWidth*sin(angle),contPoints[3][0].D()-m_nWidth*cos(angle));
			gen_region_line(&line4,contPoints[2][numR-1].D(),contPoints[3][numR-1].D(),contPoints[2][numR-1].D()+m_nWidth*sin(angle),contPoints[3][numR-1].D()-m_nWidth*cos(angle));

			concat_obj(totalRegion,lineSegL,&totalRegion);
			concat_obj(totalRegion,lineSegR,&totalRegion);
			concat_obj(totalRegion,line1,&totalRegion);
			concat_obj(totalRegion,line2,&totalRegion);
			concat_obj(totalRegion,line3,&totalRegion);
			concat_obj(totalRegion,line4,&totalRegion);		

			union1(totalRegion,&totalRegion);					
			fill_up(totalRegion,&totalRegion);					
			opening_circle(totalRegion,&totalRegion,1);	
			affine_trans_region(totalRegion, &horRegion, HomMat2DLineInv, "false");
			smallest_rectangle1(horRegion,&row1,&col1,&row2,&col2);
			//2014.6.11 nanjc 防止 0 时Bug
			if (row2==0)
			{
				return rtnInfo;
			}
			if ((row2-row1)<=20)//防止矩形太窄造成异常20130308
			{
				gen_rectangle1(&regRect,row1+1,0,row2-1,m_nWidth);
			}
			else
				gen_rectangle1(&regRect,row1+10,0,row2-10,m_nWidth);

			intersection(horRegion,regRect,&horRegion);
			double offCol = m_nWidth/2 - double(m_nColBodyCont);
			move_region(horRegion, &horRegion, HTuple(0), HTuple(offCol));		
			mirror_region(horRegion, &mirrorRegion, "column", m_nWidth); 					
			symm_difference(mirrorRegion, horRegion, &diffRegion);					
			//erosion_circle(diffRegion,&diffRegion,3.5);				
			erosion_rectangle1(diffRegion,&diffRegion,8,2);
			connection(diffRegion, &regExcludeTemp);
			select_shape(regExcludeTemp, &diffRegion, "area", "and", pBodyCont.nArea, 99999999);
			ExtExcludeAffinedDefect(rtnInfo,regExcludeTemp,diffRegion,-offCol,HomMat2DLine,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_GENERALDEFEXTS,pBodyCont.strName,true);
			count_obj(diffRegion, &nNum);
			if (nNum > 0)
			{	
				move_region(diffRegion, &diffRegion, 0, -offCol);
				affine_trans_region(diffRegion, &diffRegion, HomMat2DLine, "false");
				concat_obj(rtnInfo.regError,diffRegion,&rtnInfo.regError);
				rtnInfo.nType = ERROR_BODY_CONT;
				return rtnInfo;
			}
		}
		if (numL<10 || numR<10)
		{
			gen_rectangle1(&rtnInfo.regError,120,20,220,120);
			rtnInfo.nType = ERROR_BODY_CONT;
			rtnInfo.strEx = QObject::tr("The number of contour points is less than ten");
			return rtnInfo;	
		}		
	}
	return rtnInfo;
}
//*功能：瓶身亮斑区域
RtnInfo CCheck::fnSBriSpotReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pSBriSpotReg pSBriSpotReg = para.value<s_pSBriSpotReg>();
	s_oSBriSpotReg oSBriSpotReg = shape.value<s_oSBriSpotReg>();
	if (!pSBriSpotReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject imgReduce;
	Hobject regValid,regThresh;
	Hlong nNum;
	if (pSBriSpotReg.nShapeType == 0)
	{
		gen_region_contour_xld(oSBriSpotReg.oCheckRegion,&regValid,"filled");
	} 
	else
	{
		gen_region_contour_xld(oSBriSpotReg.oCheckRegion_Rect,&regValid,"filled");
	}	
	clip_region(regValid,&regValid,0,0,m_nHeight-1,m_nWidth-1);
	select_shape(regValid,&regValid,"area","and",100,99999999);
	count_obj(regValid,&nNum);
	if (nNum==0)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_BRI_SPOT;
		rtnInfo.strEx = QObject::tr("Please check the position of the rectangle");
		return rtnInfo;		
	}
	reduce_domain(imgSrc,regValid,&imgReduce);
	threshold(imgReduce,&regThresh,pSBriSpotReg.nGray,255);
	if (pSBriSpotReg.fOpenRadius != 0)
	{
		opening_circle(regThresh,&regThresh,pSBriSpotReg.fOpenRadius); //2017.3：开放开运算尺度(5.5)
	}	
	connection(regThresh,&regThresh);
	select_shape(regThresh,&regThresh,HTuple("area"),"and",HTuple(pSBriSpotReg.nArea),HTuple(99999999));
	count_obj(regThresh,&nNum);	
	if (nNum>0)
	{
		rtnInfo.nType = ERROR_BRI_SPOT;
		concat_obj(regThresh,rtnInfo.regError,&rtnInfo.regError);
		return rtnInfo;		
	}
	return rtnInfo;
}
//*功能：瓶底整体区域
RtnInfo CCheck::fnBAllReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	/*
	s_pBAllReg pBAllReg = para.value<s_pBAllReg>();
	s_oBAllReg oBAllReg = shape.value<s_oBAllReg>();

	double dInRadius,dOutRadius;
	Hobject regCheck,regThresh,regUnion,regInter,regBin,regConnect,regSelect,regPlor,regExcludeTemp,objSel,objSel1;
	Hobject regModeNum;
	Hobject imgReduce,imgMean;
	Hlong nNum,nLeng,nAllNum;
	double dMeanGray,dMeanGray1;
	HTuple tpArea,tpSort;
	int i;
	//模号
	if (pBAllReg.bModeNum)
	{
		gen_empty_obj(&oBAllReg.oRegModeNum);
		smallest_circle(oBAllReg.oInCircle,NULL,NULL,&dInRadius);
		smallest_circle(oBAllReg.oOutCircle,NULL,NULL,&dOutRadius);
		if (dOutRadius<dInRadius+5)
		{
			gen_rectangle1(&rtnInfo.regError,120,20,220,120);
			rtnInfo.nType = ERROR_INVALID_ROI;
			rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
			return rtnInfo;		
		}
		double dOriRow = currentOri.Row;
		double dOriCol = currentOri.Col;
		//圆心为0时，取内接圆圆心
		if (dOriRow == 0 && dOriCol == 0)
		{
			smallest_circle(oBAllReg.oInCircle,&dOriRow,&dOriCol,NULL);
			currentOri.Row = dOriRow;
			currentOri.Col = dOriCol;
		}
		double nDist = dOutRadius-dInRadius;
		mean_image(imgSrc,&imgMean,21,21);
		difference(oBAllReg.oOutCircle, oBAllReg.oInCircle, &regCheck);	
		reduce_domain(imgSrc, regCheck, &imgReduce);
		reduce_domain(imgMean,regCheck,&imgMean);
		dyn_threshold(imgReduce, imgMean, &regThresh, pBAllReg.nModeNumEdge, "dark");
		threshold(imgReduce, &regBin,0, pBAllReg.nModeNumGray);
		//取交集，字容易被分割断开；取并集，部分黑块易干扰
		union2(regThresh, regBin, &regUnion);
		intersection(regThresh, regBin, &regInter);
		//把并集中高度不满足的扣掉，再与交集取交
		//opening_circle(regThresh,&regThresh,0.5);
		closing_circle(regUnion,&regThresh,pBAllReg.fModeNumCloseSize);
		//opening_circle(regThresh,&regThresh,1.5);
		connection(regThresh,&regConnect);
		select_shape(regConnect,&regModeNum,"area","and",pBAllReg.nModeNumArea,999999);
		ExtExcludeDefect(rtnInfo,regConnect,regModeNum,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_LOF,pBAllReg.strName);				
		//select_shape(regConnect,&regModeNum,"area","and",10,999999);
		////ExtExcludeDefect(rtnInfo,regConnect,regModeNum,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_LOF,pBAllReg.strName);				
		union1(regModeNum,&regModeNum);
		//极坐标转换后判断
		polar_trans_region(regModeNum,&regPlor,dOriRow,dOriCol,0,2*PI,
			dInRadius,dOutRadius,m_nWidth,nDist,"nearest_neighbor");
		polar_trans_region(regInter,&regInter,dOriRow,dOriCol,0,2*PI,
			dInRadius,dOutRadius,m_nWidth,nDist,"nearest_neighbor");
		//closing_rectangle1(regPlor,&regPlor,11,1);
		connection(regPlor,&regPlor);
		//提取时宽度大的要适当放宽高度限制，且增加长宽比限制，防止模号连在一起被排除掉
		select_shape(regPlor, &regBin, HTuple("width").Concat("height"), "and",
			HTuple(0).Concat(0),HTuple(pBAllReg.nModeNumRadius*5).Concat(pBAllReg.nModeNumRadius*2));
		select_shape(regPlor, &regThresh, HTuple("width").Concat("height").Concat("anisometry"), "and",
			HTuple(pBAllReg.nModeNumRadius*5+1).Concat(0).Concat(3),HTuple(999999).Concat(pBAllReg.nModeNumRadius*2*1.5).Concat(999999));
		concat_obj(regThresh,regBin,&regThresh);
		//select_shape(regPlor,&regThresh,"height","and",0,pBAllReg.nModeNumRadius*2*1.5);
		ExtExcludePolarDefect(rtnInfo,regPlor,regThresh,dInRadius,dOutRadius,EXCLUDE_CAUSE_HEIGHT,EXCLUDE_TYPE_LOF,pBAllReg.strName);

		intersection(regThresh, regInter, &regThresh);
		connection(regThresh,&regConnect);

		//开运算后判断圆度，单个面积，单个宽度,高度
		opening_circle(regConnect,&regExcludeTemp,pBAllReg.fModeNumOpenSize);
		ExtExcludePolarDefect(rtnInfo,regConnect,regExcludeTemp,dInRadius,dOutRadius,EXCLUDE_CAUSE_OPEN,EXCLUDE_TYPE_LOF,pBAllReg.strName);	
		connection(regExcludeTemp,&regConnect);
		//避免极坐标变换将模号点分割的情况造成误检
		select_shape(regConnect, &regModeNum, HTuple("width").Concat("width"), "or",
			HTuple(1).Concat(m_nWidth-pBAllReg.nModeNumRadius*4),HTuple(pBAllReg.nModeNumRadius*4).Concat(m_nWidth));
		ExtExcludePolarDefect(rtnInfo,regConnect,regModeNum,dInRadius,dOutRadius,EXCLUDE_CAUSE_WIDTH,EXCLUDE_TYPE_LOF,pBAllReg.strName);				
		select_shape(regModeNum,&regThresh,"height","and",0,pBAllReg.nModeNumRadius*2);
		ExtExcludePolarDefect(rtnInfo,regModeNum,regThresh,dInRadius,dOutRadius,EXCLUDE_CAUSE_HEIGHT,EXCLUDE_TYPE_LOF,pBAllReg.strName);

		polar_trans_region_inv(regThresh,&regModeNum,dOriRow,dOriCol,0,2*PI,
			dInRadius,dOutRadius,m_nWidth,nDist,m_nWidth,m_nHeight,"nearest_neighbor");
		//变换回来后选择圆度、面积和靠边距离
		select_shape(regModeNum, &regExcludeTemp, "circularity", "and", 0.25, 1);
		ExtExcludeDefect(rtnInfo,regModeNum,regExcludeTemp,EXCLUDE_CAUSE_CIRCULARITY,EXCLUDE_TYPE_LOF,pBAllReg.strName,false);				
		select_shape(regExcludeTemp,&regThresh,"area","and",pBAllReg.nModeNumArea,999999);
		ExtExcludeDefect(rtnInfo,regExcludeTemp,regThresh,EXCLUDE_CAUSE_AREA,EXCLUDE_TYPE_LOF,pBAllReg.strName);	
		select_shape_proto(regThresh, regCheck, &regModeNum, "distance_contour",10, 10000);//2014.5.22 排除靠边
		ExtExcludeDefect(rtnInfo,regThresh,regModeNum,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_LOF,pBAllReg.strName,false);		
		count_obj(regModeNum,&nNum);
		for (i = 0;i<nNum;++i)
		{
			select_obj(regModeNum,&objSel,i+1);
			intensity(objSel,imgSrc,&dMeanGray,NULL);
			if (dMeanGray<pBAllReg.nModeNumMeanGray)
			{
				dilation_circle(objSel,&objSel1,3.5);
				intensity(objSel1,imgSrc,&dMeanGray1,NULL);
				if (dMeanGray1-dMeanGray>pBAllReg.nModeNumSubGray)
				{
					concat_obj(oBAllReg.oRegModeNum,objSel,&oBAllReg.oRegModeNum);
				}
			}
		}
		ExtExcludeDefect(rtnInfo,regModeNum,oBAllReg.oRegModeNum,EXCLUDE_CAUSE_GRAY,EXCLUDE_TYPE_LOF,pBAllReg.strName);
		count_obj(oBAllReg.oRegModeNum,&nAllNum);
		copy_obj(oBAllReg.oRegModeNum,&regExcludeTemp,1,-1);
		if (nAllNum>pBAllReg.nModeCount)
		{
			union1(oBAllReg.oRegModeNum,&regUnion);
			closing_circle(regUnion,&regModeNum,pBAllReg.fModeNumDis);
			//dilation_circle(regUnion,&regModeNum,pBAllReg.fModeNumDis);		
			//concat_obj(rtnInfo.regError, regModeNum, &rtnInfo.regError);
			//rtnInfo.nType = ERROR_LOF;
			//rtnInfo.strEx = QObject::tr("Count of ModeNumber is %1,Not %2").arg(nAllNum).arg(pBAllReg.nModeCount);
			//return rtnInfo;		

			connection(regModeNum,&regModeNum);
			area_center(regModeNum,&tpArea,NULL,NULL);
			tuple_sort_index(tpArea,&tpSort);
			tuple_length(tpArea,&nLeng);
			nAllNum = 0;
			gen_empty_region(&oBAllReg.oRegModeNum);
			for (i = 0;i<nLeng;++i)
			{
				select_obj(regModeNum,&objSel,tpSort[nLeng-i-1].I()+1);
				intersection(objSel, regUnion, &regSelect);	
				connection(regSelect,&regSelect);
				count_obj(regSelect,&nNum);
				concat_obj(oBAllReg.oRegModeNum,regSelect,&oBAllReg.oRegModeNum);
				nAllNum += nNum;
				//只选最大的
				break;
				if (nAllNum>=pBAllReg.nModeCount)
				{
					break;
				}
			}
		}
		ExtExcludeDefect(rtnInfo,regExcludeTemp,oBAllReg.oRegModeNum,EXCLUDE_CAUSE_SIDEDISTANCE,EXCLUDE_TYPE_LOF,pBAllReg.strName);
		shape.setValue(oBAllReg);		
		if (nAllNum!=pBAllReg.nModeCount)
		{
			if (nAllNum == 0)
			{
				gen_rectangle1(&rtnInfo.regError,120,20,220,120);
			}
			else
			{
				concat_obj(rtnInfo.regError, oBAllReg.oRegModeNum, &rtnInfo.regError);
			}
			rtnInfo.nType = ERROR_LOF;
			rtnInfo.strEx = QObject::tr("Count of ModeNumber is %1,Not %2").arg(nAllNum).arg(pBAllReg.nModeCount);
			return rtnInfo;		
		}
	}
	*/
	return rtnInfo;
}
//*功能：瓶底模号区域
RtnInfo CCheck::fnBModeNOReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	s_pBAllReg pBAllReg = para.value<s_pBAllReg>();
	s_oBAllReg oBAllReg = shape.value<s_oBAllReg>();
	if (!pBAllReg.bEnabled)
	{
		rtnInfo.nType = GOOD_BOTTLE;
		gen_empty_obj(&rtnInfo.regError);
		return rtnInfo;
	}

	Hobject regSobel,regTemp,regThresh,regValid,regPoint,regSel,regPrompt;//regCheck,regDynThresh,regClose,regOpen,regSelected,regBorder,regBorderSkeleton,regDiff,regExcludeTemp;
	Hobject imgPolar,imgSobel,imgMean;//,imgReduce;
	HTuple tpAreas,tpRows,tpCols;
	HTuple tpIndices;
	double dInRadius,dOutRadius;
	Hlong nNum,nValidNum,nPointNum;
	double dOriRow = currentOri.Row;
	double dOriCol = currentOri.Col;
	int i;
	double startPhi = 0;
	smallest_circle(oBAllReg.oInCircle,NULL,NULL,&dInRadius);
	smallest_circle(oBAllReg.oOutCircle,NULL,NULL,&dOutRadius);
	if (dOutRadius<dInRadius+5)
	{
		gen_rectangle1(&rtnInfo.regError,120,20,220,120);
		rtnInfo.nType = ERROR_INVALID_ROI;
		rtnInfo.strEx = QObject::tr("The inner ring should be less than the outer ring");
		return rtnInfo;		
	}
	//模号
	gen_empty_obj(&oBAllReg.oRegModeNum);
	startPhi = 0;
	polar_trans_image_ext(imgSrc,&imgPolar,dOriRow, dOriCol,startPhi,2*PI+startPhi,dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,"nearest_neighbor");  
	sobel_amp (imgPolar, &imgSobel, "sum_abs", 11);
	threshold (imgSobel, &regTemp, pBAllReg.nModePointSobel, 255);
	fill_up (regTemp, &regTemp);
	opening_circle (regTemp, &regTemp, pBAllReg.fSobelOpeningScale);
	erosion_circle (regTemp, &regSobel, 3.5);
	mean_image (imgPolar, &imgMean, pBAllReg.nMaskWidth, pBAllReg.nMaskHeight);
	dyn_threshold (imgPolar,imgMean,  &regThresh, pBAllReg.nModePointEdge, "dark");
	fill_up_shape (regThresh, &regThresh, "area", 1, 30);	//regThresh 后面应用
	opening_rectangle1 (regThresh, &regTemp, 2, 2);
	closing_rectangle1 (regTemp, &regTemp, 2, pBAllReg.nFontCloseScale);
	intersection (regTemp, regSobel, &regTemp);
	connection (regTemp, &regTemp);
	select_shape (regTemp, &regTemp, "area", "and", 20, 99999999);
	union1 (regTemp, &regTemp);
	closing_rectangle1 (regTemp, &regTemp, pBAllReg.nMaxModePointSpace, 1);
	opening_rectangle1 (regTemp, &regTemp, 10, pBAllReg.nMinModePointHeight);
	connection (regTemp, &regTemp);
	select_shape (regTemp, &regValid, HTuple("width").Concat("height"), "and", 
		HTuple(pBAllReg.nModeNOWidth).Concat(0.4*pBAllReg.nModeNOHeight),
		HTuple(1.35*pBAllReg.nModeNOWidth).Concat(1.5*pBAllReg.nModeNOHeight));
	count_obj (regValid, &nValidNum);

	//count_obj(regTemp,&nNum);
	//if (nNum>0)
	//{
	//	copy_obj(regTemp,&rtnInfo.regError,1,-1);
	///*	polar_trans_region_inv(regTemp,&rtnInfo.regError,dOriRow,dOriCol,startPhi,2*PI+startPhi,
	//		dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");*/
	//}
	//else
	//{
	//	gen_rectangle1(&rtnInfo.regError,120,20,220,120);
	//}
	//rtnInfo.nType = ERROR_LOF;
	//rtnInfo.strEx = QObject::tr("Failure to find the modeNO region!");//模号区域查找失败.
	//return rtnInfo;		

	//是否重新提取条件：1、未找到指定区域；2、区域太靠边;3、提取指定区域中模号点数严重不足
	bool bAgain = true;
	if(nValidNum>0)//条件1
	{
		area_center (regValid, NULL, NULL, &tpCols);
		for(i=0;i<nValidNum;++i)
		{
			if((tpCols[i].I() > 0.75*pBAllReg.nModeNOWidth) && (m_nWidth - tpCols[i].I() > 0.75*pBAllReg.nModeNOWidth))//条件2
			{
				bAgain = false;
				break;
			}
		}
	}
	int ntempIndex =1;
	Hlong ntempNum = 0;
	//条件3
	if (!bAgain)
	{
		/***************************/
		/*此模块判断：对不进行重提取的区域，若点数严重不足，则认为预处理区域错误，需要重提取
		/*提取点数最多的区域
		/***************************/
		gen_empty_obj (&regPoint);
		for (i = 1;i<=nValidNum;++i)
		{
			select_obj (regValid, &regSel, i);
			dilation_circle (regSel, &regSel, 5.5);
			intersection (regThresh, regSel, &regTemp);
			opening_circle (regTemp, &regTemp, pBAllReg.fModePointRadius);
			//字体开断干扰合并
			closing_rectangle1(regTemp, &regTemp, 2,pBAllReg.fModePointRadius*3);
			connection (regTemp, &regTemp);
			select_shape (regTemp, &regTemp, HTuple("area").Concat("circularity"), "and", 
				HTuple(30).Concat(0.5),HTuple(99999999).Concat(1));
			count_obj(regTemp,&nPointNum);
			if (nPointNum == pBAllReg.nModePointNum)
			{
				ntempIndex = i;
				ntempNum = nPointNum;
				copy_obj(regTemp,&regPoint,1,-1);
				break;
			}
			else
			{
				if (nPointNum>ntempNum)
				{
					ntempIndex = i;
					ntempNum = nPointNum;
					copy_obj(regTemp,&regPoint,1,-1);

				}
			}
		}
		if(ntempNum<pBAllReg.nModePointNum) //点数不足
		{
			if (pBAllReg.fModePointRadius>1.5)//是否开运算过大导致，缩小开运算尺度再试
			{
				select_obj (regValid, &regSel, ntempIndex);
				dilation_circle (regSel, &regSel, 5.5);
				intersection (regThresh, regSel, &regTemp);
				opening_circle (regTemp, &regTemp, 1.5);
				//字体开断干扰合并
				closing_rectangle1(regTemp, &regTemp,2, 5);
				connection (regTemp, &regTemp);
				select_shape (regTemp, &regPoint, HTuple("area").Concat("circularity"), "and", 
					HTuple(30).Concat(0.4),HTuple(99999999).Concat(1));
				count_obj(regPoint,&ntempNum);
				if (pBAllReg.bModeNOExt)//是否扩展找点
				{
					if(ntempNum<pBAllReg.nMinModePointNumExt) //点数仍然严重不足，少于扩展数
					{
						bAgain = true;
					}
					else
					{	
						if(ntempNum>=pBAllReg.nMinModePointNumExt && ntempNum<pBAllReg.nModePointNum)//点数仍然不够，扩展再找
						{
							select_obj (regValid, &regSel, ntempIndex);
							dilation_circle (regSel, &regSel, 5.5);
							dilation_rectangle1 (regSel, &regSel, (pBAllReg.nModePointNum-ntempNum)*pBAllReg.nSigPointDisExt, 1);
							intersection (regThresh, regSel, &regTemp);
							opening_circle (regTemp, &regTemp, 1.5);
							//字体开断干扰合并
							closing_rectangle1(regTemp, &regTemp,2, 5);
							connection (regTemp, &regTemp);
							select_shape (regTemp, &regPoint, HTuple("area").Concat("circularity"), "and", 
								HTuple(30).Concat(0.35),HTuple(99999999).Concat(1));
							count_obj(regPoint,&ntempNum);
							if (ntempNum<pBAllReg.nModePointNum)
							{
								bAgain = true;
							}
						}
					}
				}
				else
				{
					if(ntempNum<pBAllReg.nModePointNum) //点数不足且不扩展
					{
						bAgain = true;
					}
				}
				
			}
			else//开运算尺度很小，直接重新提取
			{
				bAgain = true;
			}
		}
	   /*判断模块终止
	   /**********************************/
	}
	//膜点在0点位置被断开，改变初始角度重新提取
	if (bAgain)
	{
		startPhi = PI/2;
		polar_trans_image_ext(imgSrc,&imgPolar,dOriRow, dOriCol,startPhi,2*PI+startPhi,dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,"nearest_neighbor");  
		sobel_amp (imgPolar, &imgSobel, "sum_abs", 11);
		threshold (imgSobel, &regTemp, pBAllReg.nModePointSobel, 255);
		fill_up (regTemp, &regTemp);
		opening_circle (regTemp, &regTemp, pBAllReg.fSobelOpeningScale);
		erosion_circle (regTemp, &regSobel, 3.5);
		mean_image (imgPolar, &imgMean, pBAllReg.nMaskWidth, pBAllReg.nMaskHeight);
		dyn_threshold (imgPolar,imgMean,  &regThresh, pBAllReg.nModePointEdge, "dark");
		fill_up_shape (regThresh, &regThresh, "area", 1, 30);	//regThresh 后面应用
		opening_rectangle1 (regThresh, &regTemp, 2, 2);
		closing_rectangle1 (regTemp, &regTemp, 2, pBAllReg.nFontCloseScale);
		intersection (regTemp, regSobel, &regTemp);
		connection (regTemp, &regTemp);
		select_shape (regTemp, &regTemp, "area", "and", 20, 99999999);
		union1 (regTemp, &regTemp);
		closing_rectangle1 (regTemp, &regTemp, pBAllReg.nMaxModePointSpace, 1);
		opening_rectangle1 (regTemp, &regTemp, 10, pBAllReg.nMinModePointHeight);
		connection (regTemp, &regPrompt);
		select_shape (regPrompt, &regValid, HTuple("width").Concat("height"), "and", 
			HTuple(pBAllReg.nModeNOWidth).Concat(0.4*pBAllReg.nModeNOHeight),
			HTuple(1.35*pBAllReg.nModeNOWidth).Concat(1.5*pBAllReg.nModeNOHeight));
		count_obj (regValid, &nValidNum);
		if(nValidNum<1)
		{
			count_obj(regPrompt,&nNum);
			if (nNum>0)
			{
				polar_trans_region_inv(regPrompt,&rtnInfo.regError,dOriRow,dOriCol,startPhi,2*PI+startPhi,
					dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");
			}
			else
			{
				gen_rectangle1(&rtnInfo.regError,120,20,220,120);
			}
			shape.setValue(oBAllReg);		
			rtnInfo.nType = ERROR_LOF;
			rtnInfo.strEx = QObject::tr("Failure to find the modeNO region!");//模号区域查找失败.
			return rtnInfo;		
		}
		//提取点数最多的区域
		ntempIndex =1;
		ntempNum = 0;
		gen_empty_obj (&regPoint);
		count_obj (regValid, &nValidNum);
		for (i = 1;i<=nValidNum;++i)
		{
			select_obj (regValid, &regSel, i);
			dilation_circle (regSel, &regSel, 5.5);
			intersection (regThresh, regSel, &regTemp);
			opening_circle (regTemp, &regTemp, pBAllReg.fModePointRadius);
			//字体开断干扰合并
			closing_rectangle1(regTemp, &regTemp,2, pBAllReg.fModePointRadius*3);
			connection (regTemp, &regTemp);
			select_shape (regTemp, &regTemp, HTuple("area").Concat("circularity"), "and", 
				HTuple(30).Concat(0.5),HTuple(99999999).Concat(1));
			count_obj(regTemp,&nPointNum);
			if (nPointNum == pBAllReg.nModePointNum)
			{
				ntempIndex = i;
				ntempNum = nPointNum;
				copy_obj(regTemp,&regPoint,1,-1);
				break;
			}
			else
			{
				if (nPointNum>ntempNum)
				{
					ntempIndex = i;
					ntempNum = nPointNum;
					copy_obj(regTemp,&regPoint,1,-1);
				}
			}
		}
		//个数不足降低开运算尺度
		if(ntempNum<pBAllReg.nModePointNum)
		{
			if (pBAllReg.fModePointRadius>1.5)//是否开运算过大导致，缩小开运算尺度再试
			{
				select_obj (regValid, &regSel, ntempIndex);
				dilation_circle (regSel, &regSel, 5.5);
				intersection (regThresh, regSel, &regTemp);
				opening_circle (regTemp, &regTemp, 1.5);
				//字体开断干扰合并
				closing_rectangle1(regTemp, &regTemp,2, 5);
				connection (regTemp, &regTemp);
				select_shape (regTemp, &regPoint, HTuple("area").Concat("circularity"), "and", 
					HTuple(30).Concat(0.4),HTuple(99999999).Concat(1));
				count_obj(regPoint,&ntempNum);
				if (pBAllReg.bModeNOExt)//是否扩展找点
				{
					if(ntempNum<pBAllReg.nMinModePointNumExt) //点数仍然严重不足，少于扩展数
					{
						if (ntempNum>0)
						{
							polar_trans_region_inv(regPoint,&rtnInfo.regError,dOriRow,dOriCol,startPhi,2*PI+startPhi,
								dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");
						}
						else
						{
							gen_rectangle1(&rtnInfo.regError,120,20,220,120);
						}
						shape.setValue(oBAllReg);		
						rtnInfo.nType = ERROR_LOF;
						rtnInfo.strEx = QObject::tr("The count of the mode points isn't error,Found [%1]!").arg(ntempNum);//膜点个数错误，找到%1个！
						return rtnInfo;	
					}
					else
					{	
						if(ntempNum>=pBAllReg.nMinModePointNumExt && ntempNum<pBAllReg.nModePointNum)//点数仍然不够，扩展再找
						{
							select_obj (regValid, &regSel, ntempIndex);
							dilation_circle (regSel, &regSel, 5.5);
							dilation_rectangle1 (regSel, &regSel, (pBAllReg.nModePointNum-ntempNum)*pBAllReg.nSigPointDisExt, 1);
							intersection (regThresh, regSel, &regTemp);
							opening_circle (regTemp, &regTemp, 1.5);
							//字体开断干扰合并
							closing_rectangle1(regTemp, &regTemp,2, 5);
							connection (regTemp, &regTemp);
							select_shape (regTemp, &regPoint, HTuple("area").Concat("circularity"), "and", 
								HTuple(30).Concat(0.35),HTuple(99999999).Concat(1));
							count_obj(regPoint,&ntempNum);
							if (ntempNum<pBAllReg.nModePointNum)
							{
								if (ntempNum>0)
								{
									polar_trans_region_inv(regPoint,&rtnInfo.regError,dOriRow,dOriCol,startPhi,2*PI+startPhi,
										dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");
								}
								else
								{
									gen_rectangle1(&rtnInfo.regError,120,20,220,120);
								}
								shape.setValue(oBAllReg);
								rtnInfo.nType = ERROR_LOF;
								rtnInfo.strEx = QObject::tr("The count of the mode points isn't error,Found [%1]!").arg(ntempNum);//膜点个数错误，找到%1个！
								return rtnInfo;	
							}
						}
					}
				}
				else
				{
					if(ntempNum<pBAllReg.nModePointNum) //点数不足且不扩展
					{
						if (ntempNum>0)
						{
							polar_trans_region_inv(regPoint,&rtnInfo.regError,dOriRow,dOriCol,startPhi,2*PI+startPhi,
								dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");
						}
						else
						{
							gen_rectangle1(&rtnInfo.regError,120,20,220,120);
						}
						shape.setValue(oBAllReg);		
						rtnInfo.nType = ERROR_LOF;
						rtnInfo.strEx = QObject::tr("The count of the mode points isn't error,Found [%1]!").arg(ntempNum);//膜点个数错误，找到%1个！
						return rtnInfo;	
					}
				}

			}
			else//开运算尺度很小，直接返回
			{
				if (ntempNum>0)
				{
					polar_trans_region_inv(regPoint,&rtnInfo.regError,dOriRow,dOriCol,startPhi,2*PI+startPhi,
						dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");
				}
				else
				{
					gen_rectangle1(&rtnInfo.regError,120,20,220,120);
				}
				shape.setValue(oBAllReg);		
				rtnInfo.nType = ERROR_LOF;
				rtnInfo.strEx = QObject::tr("The count of the mode points isn't error,Found [%1]!").arg(ntempNum);//膜点个数错误，找到%1个！
				return rtnInfo;	
			}
		}
	}
		
	if(ntempNum>pBAllReg.nModePointNum)
	{			   
		area_center (regPoint, &tpAreas, NULL, NULL);
		tuple_sort_index (tpAreas, &tpIndices);
		copy_obj(regPoint,&regTemp,1,-1);
		gen_empty_obj (&regPoint);
		for (i = ntempNum-1;i>=ntempNum-pBAllReg.nModePointNum;--i)
		{
			select_obj (regTemp, &regSel, tpIndices[i].I()+1);
			concat_obj (regPoint, regSel, &regPoint);
		}
	}
	polar_trans_region_inv(regPoint,&oBAllReg.oRegModeNum,dOriRow,dOriCol,startPhi,2*PI+startPhi,
		dInRadius,dOutRadius,m_nWidth,dOutRadius-dInRadius,m_nWidth,m_nHeight,"nearest_neighbor");
	shape.setValue(oBAllReg);		
	return rtnInfo;
}

//*功能：瓶底模点模号识别-2017.7
RtnInfo CCheck::fnBMouldNOReg(Hobject &imgSrc,QVariant &para,QVariant &shape)
{
	RtnInfo rtnInfo;
	rtnInfo.nType = GOOD_BOTTLE;
	gen_empty_obj(&rtnInfo.regError);
	s_pBAllReg pBAllReg = para.value<s_pBAllReg>();
	s_oBAllReg oBAllReg = shape.value<s_oBAllReg>();
	oBAllReg.nMouldNO = -1;
	oBAllReg.strMouldInfo = "";
	bool debug = 0;//pBAllReg.bCheckIfReportError;//0;//
	if (!pBAllReg.bEnabled)
	{
		return rtnInfo;
	}
	int i_mouldnum = 7;//默认11模号参数，如果是13模号，则其值为9
	if(pBAllReg.bFlagThirtMp)  i_mouldnum = 9;
	Hobject imgReduce,imgMean,RegDyn,RegCon,RegSelTemp,RegSel;
	Hobject RegRound,Cont,RegMould,ObjSel;
	Hobject RegConLt,RegClosed;//liuxu 2018-3-12
	Hobject RegMouldUnion,RegAffineTrans,RegAffineCon,RegClosing,RegCloCon,RegSort;
	Hobject RegInter,RegInterCon,ObjSel_Left,ObjSel_Right,RegShapeTrans,RegAffineUnion;
	HTuple OutCirRow,OutCirCol,OrderNum,num,Row,Col,RowBegin,ColBegin,RowEnd,ColEnd;
	HTuple RowCen,ColCen,LinePhi,ObjRow,ObjCol,TempPhi,PhiDiff,DefDiff;
	HTuple nCount,Row1,Column1,Phi,HomMat2D,RegMinDist,countRound,DegDiff,nMouldArea;
	HTuple nWid, Row0, Column0, Phi0, lenW0, lenH0;//liuxu
	Hobject RegMouldRect,RegMould1;
	double Degree;

	rtnInfo.nMouldID = oBAllReg.nMouldNO;
	smallest_circle(oBAllReg.oOutCircle,&OutCirRow,&OutCirCol,NULL);
	gen_empty_obj(&oBAllReg.oRegMould);
	gen_empty_obj(&RegMould);
	OrderNum = HTuple();
	nMouldArea=3.14*pBAllReg.nMouldDia*pBAllReg.nMouldDia;
	// 模点提取(提取7个点)
	reduce_domain(imgSrc, oBAllReg.oOutCircle, &imgReduce);
	mean_image (imgReduce, &imgMean, 51, 51);
	dyn_threshold (imgReduce, imgMean, &RegDyn, pBAllReg.nMouldEdge,"dark");
	connection (RegDyn, &RegCon);
	/************输出印花***************************/
	if(pBAllReg.bDelStripe)
	{
		Hobject RegRectRem;
		closing_circle(RegDyn, &RegRectRem, 10); //remove the 印花
		connection (RegRectRem, &RegRectRem);
		shape_trans(RegRectRem, &RegRectRem,"rectangle2");
		select_shape(RegRectRem, &RegRectRem, "rectangularity","and", 0.8, 1);
		select_shape(RegRectRem, &RegRectRem, HTuple("ra").Concat("rb"), "and", HTuple(pBAllReg.nStrLengthth*0.7/2).Concat(pBAllReg.nStrWidth*0.7/2), HTuple(pBAllReg.nStrLengthth*1.5/2).Concat(pBAllReg.nStrWidth*1.5/2)); // 印花面积（选择项）
		union2(RegRectRem,RegCon,&RegCon);
		closing_circle(RegCon, &RegCon, 3); //inorder to remain the one mould has two part
		connection (RegCon, &RegCon);
	}
/*****************************************/
 /*select_shape (RegCon, &RegCon, "area", "and", 20, 99999999); 
     //for the large anisometry to opening operator  2018-3-12 liuxu
    select_shape(RegCon, &RegConLt, "anisometry", "and", 2.0, 10);//choose the large ra
    opening_circle(RegConLt, &RegClosed, 3) ;
    select_shape (RegClosed, &RegClosed, HTuple("width").Concat("height"),"or",HTuple(pBAllReg.nMouldDia*0.6).Concat(pBAllReg.nMouldDia*0.6), HTuple(pBAllReg.nMouldDia*1.2).Concat(pBAllReg.nMouldDia*1.2));
    union1(RegClosed,&RegClosed);
    select_shape (RegCon, &RegCon, "anisometry", "and", 1, 2);
    select_shape (RegCon, &RegSel, HTuple("width").Concat("height"),"or",HTuple(pBAllReg.nMouldDia*0.6).Concat(pBAllReg.nMouldDia*0.6), HTuple(pBAllReg.nMouldDia*1.2).Concat(pBAllReg.nMouldDia*1.2));
    union1(RegSel,&RegSel);
    union2 (RegSel,RegClosed ,&RegSel);
    connection (RegSel, &RegSel); */
/**************rough choose***************************/
	select_shape (RegCon,&RegCon,"area", "and", 50,1500); //3.14*pBAllReg.nMouldDia*pBAllReg.nMouldDia*0.4/pi*r^2*0.4 liuxu  mould point pix < 500,must large .it maybe include 闷头印   ,the value is decide by mould size
	select_shape (RegCon, &RegCon, HTuple("ra").Concat("rb"), "and", HTuple(3).Concat(3), HTuple(pBAllReg.nMouldDia*2).Concat(pBAllReg.nMouldDia)); //ra and rb, ra to avoid the 闷头印连接模点被删除
 //for the large anisometry to opening operator  2018-3-12 liuxu(排除闷头印干扰)
    select_shape(RegCon,&RegConLt,"anisometry", "and", 3.5, 9999); //2.5->2.8
    opening_circle(RegConLt, &RegClosed, 3); 
    connection(RegClosed, &RegClosed);//avoid exist two segment
    select_shape(RegClosed, &RegClosed,HTuple("width").Concat("height"),"or",HTuple(pBAllReg.nMouldDia*0.6).Concat(pBAllReg.nMouldDia*0.6), HTuple(pBAllReg.nMouldDia*1.2).Concat(pBAllReg.nMouldDia*1.2));
    select_shape (RegClosed, &RegClosed, "anisometry", "and", 1, 3);
	union1(RegClosed,&RegClosed);
    
    select_shape (RegCon, &RegCon,"anisometry","and",1, 3.444449);//key parameter
    union1(RegCon,&RegCon);
    union2 (RegCon,RegClosed ,&RegCon);
    connection (RegCon, &RegCon);
	/*************refine choose****************************/
	select_shape (RegCon, &RegCon, "area", "and", 50,  1.2*nMouldArea);
    select_shape (RegCon, &RegCon, HTuple("ra").Concat("rb")/*.Concat("max_diameter")*/, "and",  HTuple(3).Concat(3)/*.Concat(3)*/ , HTuple(pBAllReg.nMouldDia*1.2).Concat(pBAllReg.nMouldDia)/*.Concat(pBAllReg.nMouldDia*1.2)*/); //ra and rb
	select_shape (RegCon, &RegCon, "contlength","or",10, 2*3.14*pBAllReg.nMouldDia/*2*3.14*pBAllReg.nMouldDia*/);//2pi*r 200 contlength of mould point
    
    shape_trans(RegCon,&RegCon,"convex");
    select_shape (RegCon, &RegCon, "outer_radius", "and", 5, pBAllReg.nMouldDia*1.5/2);
	select_shape (RegCon, &RegCon, HTuple("ra").Concat("rb"), "and", HTuple(4).Concat(4), HTuple(pBAllReg.nMouldDia*1.5).Concat(pBAllReg.nMouldDia));
    select_shape (RegCon, &RegCon, "anisometry", "and", 1, 3);//reselect
    select_shape (RegCon, &RegCon, "contlength", "or", 10, 2*3.14*pBAllReg.nMouldDia*1.5/2);
    select_shape (RegCon, &RegCon, "area", "and", 60, 9999999); 
    select_shape (RegCon, &RegSel, HTuple("width").Concat("height"),"or",HTuple(pBAllReg.nMouldDia*0.4).Concat(pBAllReg.nMouldDia*0.4), HTuple(pBAllReg.nMouldDia*1.5).Concat(pBAllReg.nMouldDia*1.5));
	if(debug)
	{
		write_region(RegCon,".\\RegCon.hobj");
		write_region(RegSel,".\\RegSel.hobj");
	}
/******************************************/
	//ExtExcludeDefect(rtnInfo,RegCon,RegSel,EXCLUDE_CAUSE_MOULD_DIAMETER,EXCLUDE_TYPE_MOULD_POINT,pBAllReg.strName);
	/*if (pBAllReg.nMouldInnerDiaH > pBAllReg.nMouldInnerDiaL)
	{
		select_shape (RegSel, &RegSelTemp, "inner_radius", "and", pBAllReg.nMouldInnerDiaL/2, pBAllReg.nMouldInnerDiaH/2);
		ExtExcludeDefect(rtnInfo,RegSel,RegSelTemp,EXCLUDE_CAUSE_INRADIUS,EXCLUDE_TYPE_MOULD_POINT,pBAllReg.strName);
		copy_obj (RegSelTemp, &RegSel, 1, -1);
	}*/
	count_obj (RegSel, &num);
	nWid = 0.8*(pBAllReg.nMouldDia*11+pBAllReg.nMouldSpace*10); //liuxu

	if (num[0].I() >= i_mouldnum-2)
	{
		//其他方式找模点（每两点生成一条线，查找其他点是否在这条线上）
		//注释掉的拟合直线方法不稳定
		int count = 0;
		HTuple  dispp,nsmallDif;
		nsmallDif = 0.8*(pBAllReg.nMouldSpace + pBAllReg.nMouldDia);
		area_center (RegSel, NULL, &Row, &Col); 
		for (int i=0;i<=Row.Num()-2;i++)
		{
			RowBegin = Row[i];
			ColBegin = Col[i];
		    for (int j=i+1;j<=Row.Num()-1;j++)
			{
				RowEnd = Row[j];
				ColEnd = Col[j];
				distance_pp(RowBegin,ColBegin,RowEnd,ColEnd,&dispp);//protect the calculate point distance,revise 20180731
                if(dispp<nsmallDif)
                    continue;
                
				RowCen = (RowBegin+RowEnd)/2;
				ColCen = (ColBegin+ColEnd)/2;
				count = 0; //模点计数清零
				gen_empty_obj (&RegMould);
				for (int k=0;k<=Row.Num()-1;k++)
				{
					angle_ll (RowBegin,ColBegin,RowEnd,ColEnd,RowCen,ColCen,Row[k],Col[k],&PhiDiff);
					tuple_deg (PhiDiff, &DegDiff);
					tuple_fabs (DegDiff, &DegDiff);
					if (DegDiff > 90)
					{
						Degree = 180-DegDiff[0].D();
					}			
					else
					{
						Degree = DegDiff[0].D();
					}

					if (Degree < 5) //#10
					{
						select_obj (RegSel, &ObjSel, k+1);
						concat_obj (RegMould, ObjSel, &RegMould);
						count = count+1;
					}                   
				}
			    if (count == i_mouldnum)//7
				{   //break;//
				   union1(RegMould,&RegMould1);
                   shape_trans(RegMould1, &RegMouldRect,"rectangle2");
                   smallest_rectangle2(RegMouldRect, &Row0, &Column0, &Phi0, &lenW0, &lenH0);
                   if(lenH0<0.4*pBAllReg.nMouldDia || lenW0<nWid *0.4)//||lenH0>0.6*pBAllReg.nMouldDia || lenW0>0.7*nWid,,,,,,,revise 20180731
                   {
				       continue;
                       count=0;
					}
                   else
                       break;
				 }
			}//endfor j
		    if (count == i_mouldnum)
			   break;
		}//endfor i
		if(debug)
		{
			write_object(RegMould,".\\RegMould.hobj");
		}
		count_obj(RegMould, &nCount);
		copy_obj (RegMould, &oBAllReg.oRegMould, 1, -1);
		shape.setValue(oBAllReg);
		if (nCount != i_mouldnum) //模点数不等于7
		{
			if (pBAllReg.bCheckIfReportError)
			{
				copy_obj (oBAllReg.oOutCircle, &rtnInfo.regError, 1, -1);
				rtnInfo.nType = ERROR_MOULD_POINT;
				rtnInfo.strEx = QObject::tr("Mould point number not equal seven!"); 
			}
			return rtnInfo;	
		}
		else //提取正常:7个点
		{		
			HTuple lenW,lenH,temp;
			HTuple PointA_Row,PointA_Col,PointB_Row,PointB_Col,PointCir_Row,PointCir_Col;
			union1 (RegMould, &RegMouldUnion);
			smallest_rectangle2 (RegMouldUnion, &Row1, &Column1, &Phi, &lenW, &lenH);
			//nWid := 0.8*(nMouldDia*11+nMouldSpace*10) //liuxu
   //         if(lenW<nWid/2)
   //             nType:=-1
   //         endif
			PointA_Row = Row1+lenW*Phi.Sin();
			PointA_Col = Column1-lenW*Phi.Cos();
			PointB_Row = Row1-lenW*Phi.Sin();
			PointB_Col = Column1+lenW*Phi.Cos();
			PointCir_Row = OutCirRow; //外圆圆心
			PointCir_Col = OutCirCol;
			//temp>0是顺时针，temp<0是逆时针，temp=0三点共线
			temp = (PointB_Col-PointA_Col)*(PointCir_Row-PointB_Row)-(PointB_Row-PointA_Row)*(PointCir_Col-PointB_Col);
	        if(pBAllReg.bCheckAntiClockwise == true) //逆时针
			{
				if(temp < 0) //A->B->圆心的顺序为逆时针
				{
					RowBegin = PointA_Row;
					ColBegin = PointA_Col;
					RowEnd = PointB_Row;
					ColEnd = PointB_Col; 
				}                  
		        else
				{
					RowBegin = PointB_Row;
					ColBegin = PointB_Col;
					RowEnd = PointA_Row;
					ColEnd = PointA_Col;
				}
			}
			else //顺时针
			{
				 if(temp < 0) //A->B->圆心的顺序为逆时针
				 {
					RowBegin = PointB_Row;
					ColBegin = PointB_Col;
					RowEnd = PointA_Row;
					ColEnd = PointA_Col; 
				 }
				 else
				 {
					RowBegin = PointA_Row;
					ColBegin = PointA_Col;
					RowEnd = PointB_Row;
					ColEnd = PointB_Col;
				 }
			}
			angle_lx (RowBegin,ColBegin,RowEnd,ColEnd,&Phi);
			vector_angle_to_rigid (Row1, Column1, Phi, Row1, Column1, 0, &HomMat2D);
			connection (RegMouldUnion, &RegAffineCon);
			affine_trans_region (RegAffineCon, &RegAffineTrans, HomMat2D, "nearest_neighbor"); //2018.3-必需先connection再affine
			shape_trans (RegAffineTrans, &RegShapeTrans, "rectangle1");
			union1 (RegAffineTrans, &RegAffineUnion); 
			closing_circle (RegAffineUnion, &RegClosing, pBAllReg.nMouldSpace*2);//2018-3-12  liuxu for some little circle //1.5->2只有半个圆时1.5无法连接
			connection (RegClosing, &RegCloCon);
			sort_region (RegCloCon, &RegSort,"upper_left", "true", "column"); //从左到右排好序
			if(debug)
			{
				write_region(RegCloCon,".\\RegCloCon.hobj");
			}
			count_obj (RegSort, &num);
			if (num < 3) //相邻模点连接后，至少会分成3组
			{		
				if (pBAllReg.bCheckIfReportError)
				{
					rtnInfo.nType = ERROR_MOULD_POINT;
					copy_obj (oBAllReg.oOutCircle, &rtnInfo.regError, 1, -1);						
				}
				return rtnInfo;	
			}
			else
			{
				int i;
				for (i=1;i<=num;i++)
				{
					select_obj (RegSort, &ObjSel, i);
					intersection (ObjSel, RegShapeTrans, &RegInter); 
					connection (RegInter, &RegInterCon);
					select_shape (RegInterCon, &RegInterCon, "area", "and", 10, 99999999);//2018-3-12 liuxu remove small outlier
					count_obj (RegInterCon, &nCount);
					tuple_concat (OrderNum, nCount, &OrderNum);			
				}
			    // 记录模点之间的间隔信息
				Hlong test = num[0].L();
				for (i=1;i<=num-1;i++)
				{
					select_obj (RegSort, &ObjSel_Left, i);
					select_obj (RegSort, &ObjSel_Right, i+1);
					distance_rr_min (ObjSel_Left, ObjSel_Right, &RegMinDist, NULL, NULL, NULL, NULL);
					if (pBAllReg.nMouldDia+pBAllReg.nMouldSpace != 0)
					{
						nCount = (RegMinDist-pBAllReg.nMouldSpace)/(pBAllReg.nMouldDia+pBAllReg.nMouldSpace);
						tuple_round (nCount, &countRound);
						if(countRound==0)//protect 
							countRound = 1; 
						tuple_insert (OrderNum, i*2-1, countRound, &OrderNum);					
					}					
				}
				// tuple数组转换成qstring
				QString strMoldOrder="";
				int len = OrderNum.Num();
				if(debug)
				{
				    HTuple Orders,FileHandle;
					tuple_string(OrderNum,"0",&Orders);
					open_file(".//result.txt", "output", &FileHandle);
					fwrite_string(FileHandle,  Orders);
					fnew_line(FileHandle);
					close_file(FileHandle);	
				}
				int stind = 0;				
				if (len >= 5) //模点排序至少为5位数
				{
					if(pBAllReg.bFlagThirtMp)
					{
						stind = 1;
						len = len - 1;
						strMoldOrder += QString("%1").arg(OrderNum[0].I()-1);
					}
					for (int i=stind; i<len;i++)
					{
						strMoldOrder += QString("%1").arg(OrderNum[i].I());
					}
					if(pBAllReg.bFlagThirtMp)
					{
					    strMoldOrder += QString("%1").arg(OrderNum[len].I()-1);
					}
					//if(pBAllReg.bFlagThirtMp)

					// 返回对应的模号01-90
					int moldID;
					moldID = strMoldOrderList.indexOf(strMoldOrder);
					oBAllReg.nMouldNO = moldID==-1?-1:moldID+1;
					oBAllReg.strMouldInfo = strMoldOrder;
					if (moldID == -1)
					{
						// 2018.3---在模号表中找不到对应模号时，找出最相似的模号
						QList<QString>::Iterator itbeg = strMoldOrderList.begin(),itend = strMoldOrderList.end();
						int i = 0;
						int nSim = 99999;
						for (;itbeg != itend; itbeg++,i++)
						{
							if (itbeg->length() == len) //长度相同
							{  //相似度最高
								QString strTemp;
								int nSimTemp;
								strTemp = strMoldOrderList[i];
								nSimTemp = strDistance(strTemp.toStdString(),strMoldOrder.toStdString()); 
								if (nSimTemp < nSim)
								{
									nSim = nSimTemp;
									moldID = i;
								}
							}
						}
						oBAllReg.nMouldNO = moldID==-1?-1:moldID+1;
						if (oBAllReg.nMouldNO == -1){
							oBAllReg.strMouldInfo = oBAllReg.strMouldInfo+"\n"+tr(" No mouldID can match the arrange order!");}
						else{
						    oBAllReg.strMouldInfo = oBAllReg.strMouldInfo+"\n"+tr(" Find the most similar mold ID!");}
					}
				}//if (len >= 5) //模点排序至少为5位数
			} 
		}
	}
	
	shape.setValue(oBAllReg);
	rtnInfo.nMouldID = oBAllReg.nMouldNO;
//	if(debug)
//	{
//	tuple_string(ind,'0',indexnum)
//    tuple_string(OrderNum,'0',Orders)
//    fwrite_string (FileHandle, indexnum)
//    fwrite_string (FileHandle, ' ' + Orders)
//*     fwrite_string (FileHandle, OrderNum)
//    fnew_line (FileHandle)
//	}
	// 模号小于0，识别失败
	if (oBAllReg.nMouldNO < 0)
	{
		if (pBAllReg.bCheckIfReportError)
		{			
			copy_obj (oBAllReg.oOutCircle, &rtnInfo.regError, 1, -1);
			rtnInfo.nType = ERROR_MOULD_POINT;
			rtnInfo.strEx = QObject::tr("Mould point number identification error!");
		}				
		return rtnInfo;	
	}
	else 
	{ 
		// 识别正确，判断是否要剔除
		if (!pBAllReg.strModeReject.isEmpty())
		{
			QStringList strTemp;
			strTemp = pBAllReg.strModeReject.split(",");
			for (int i=0;i<strTemp.size();i++)
			{
				if (strTemp.at(i) == "1" && i+1==oBAllReg.nMouldNO) //模号被标记且与识别的号码一致
				{
					rtnInfo.nType = ERROR_MOULD_REJECT;
					copy_obj (oBAllReg.oOutCircle, &rtnInfo.regError, 1, -1);
					return rtnInfo;
				}
			}

		}

	}
	return rtnInfo;
}

//2018.3-求两个字符串的相似度 Levenshtein Distance(编辑距离法)
int CCheck::strDistance(const string source,const string target)
{
	//step 1
	int n=source.length();
	int m=target.length();
	if (m==0) return n;
	if (n==0) return m;
	//Construct a matrix
	typedef vector< vector<int> >  Tmatrix;
	Tmatrix matrix(n+1);
	for(int i=0; i<=n; i++)  matrix[i].resize(m+1);

	//step 2 Initialize
	for(int i=1;i<=n;i++) matrix[i][0]=i;
	for(int i=1;i<=m;i++) matrix[0][i]=i;

	//step 3
	for(int i=1;i<=n;i++)
	{
		const char si=source[i-1];
		//step 4
		for(int j=1;j<=m;j++)
		{
			const char dj=target[j-1];
			//step 5
			int cost;
			if(si==dj){
				cost=0;
			}
			else{
				cost=1;
			}
			//step 6
			const int above=matrix[i-1][j]+1;
			const int left=matrix[i][j-1]+1;
			const int diag=matrix[i-1][j-1]+cost;
			matrix[i][j]=min(above,min(left,diag));

		}
	}
	
	//step7
	return matrix[n][m];
}