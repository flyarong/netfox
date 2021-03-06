#include "Stdafx.h"
#include "GameOption.h"
#include "GameClient.h"
#include "GameClientDlg.h"

#include "FlyGameSound.h"

CFlyGameSound  g_myCFlyGameSound;
//////////////////////////////////////////////////////////////////////////
//宏定义

//游戏定时器
#define IDI_OUT_CARD					200								//出牌定时器
#define IDI_MOST_CARD					201								//最大定时器
#define IDI_START_GAME					202								//开始定时器
#define IDI_LAND_SCORE					203								//叫分定时器

#define ID_SHOW_BOMB				108					//炸弹
#define ID_SHOW_PLANE				109					//飞机
#define ID_SHOW_ROCKET				110					//火箭


#define IDI_QIANG_LAND					204							//抢地主定时器
//////////////////////////////////////////////////////////////////////////

int g_PrintLogFile=1;
void WriteLog(CString strFileName, CString strText)
{
	//判断是否打印日志文件
	if ( g_PrintLogFile != 1)
		return;

	try
	{
		CTime tm = CTime::GetCurrentTime();
		CString strTime = tm.Format(_T("%Y-%m-%d %H:%M:%S"));
		//BOOL bFull = FALSE;
		CStdioFile file;
		if( file.Open(strFileName, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeReadWrite) != 0)
		{
			file.SeekToEnd();
			file.WriteString(strTime);
			file.WriteString(strText);
			file.WriteString(_T("\n\n"));
			//if(file.GetLength() > 2000000)
			//	bFull = TRUE;
			file.Close();
		}
		/*
		if(!bFull) return;
		if( file.Open(strFileName, CFile::modeCreate|CFile::modeReadWrite) != 0)
		{
		file.SeekToEnd();
		file.WriteString(strTime);
		file.WriteString(strText);
		file.WriteString(_T("\n"));
		file.Close();
		}
		*/
	}
	catch(...)
	{
	}
}


BEGIN_MESSAGE_MAP(CGameClientDlg, CGameFrameDlg)
	ON_WM_TIMER()
	ON_MESSAGE(IDM_START,OnStart)
	ON_MESSAGE(IDM_OUT_CARD,OnOutCard)
	ON_MESSAGE(IDM_PASS_CARD,OnPassCard)
	ON_MESSAGE(IDM_LAND_SCORE,OnLandScore)	
	ON_MESSAGE(IDM_QIANG_LAND,OnQiangLand)
	ON_MESSAGE(IDM_AUTO_OUTCARD,OnAutoOutCard)
	ON_MESSAGE(IDM_LEFT_HIT_CARD,OnLeftHitCard)
	ON_MESSAGE(IDM_RIGHT_HIT_CARD,OnRightHitCard)
	ON_MESSAGE(IDM_RESET_UI,OnResetUI )
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

//构造函数
CGameClientDlg::CGameClientDlg() : CGameFrameDlg(&m_GameClientView)
{
	//游戏变量
	uiShowInt = 0;
	m_wBombTime=1;
	m_bHandCardCount=0;
	m_wLandUser=INVALID_CHAIR;
	memset(m_bCardCount,0,sizeof(m_bCardCount));
	memset(m_bHandCardData,0,sizeof(m_bHandCardData));

	//
	m_wXNum = 1;

	//配置变量
	m_bDeasilOrder=false;
	m_dwCardHSpace=DEFAULT_PELS;

	//出牌变量
	m_bTurnCardCount=0;
	m_bTurnOutType=CT_INVALID;
	memset(m_bTurnCardData,0,sizeof(m_bTurnCardData));

	//辅助变量
	m_wTimeOutCount=0;
	m_wMostUser=INVALID_CHAIR;

	return;
}

//析构函数
CGameClientDlg::~CGameClientDlg()
{
}

//初始函数
bool CGameClientDlg::InitGameFrame()
{
	//设置标题
	SetWindowText(TEXT("疯狂斗地主游戏"));

	//设置图标
	HICON hIcon=LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME));
	SetIcon(hIcon,TRUE);
	SetIcon(hIcon,FALSE);

	//读取配置
	m_dwCardHSpace=AfxGetApp()->GetProfileInt(TEXT("GameOption"),TEXT("CardSpace"),DEFAULT_PELS);
	m_bDeasilOrder=AfxGetApp()->GetProfileInt(TEXT("GameOption"),TEXT("DeasilOrder"),FALSE)?true:false;

	//调整参数
	if ((m_dwCardHSpace>MAX_PELS)||(m_dwCardHSpace<LESS_PELS)) m_dwCardHSpace=DEFAULT_PELS;

	//配置控件
	m_GameClientView.SetUserOrder(m_bDeasilOrder);
	m_GameClientView.m_HandCardControl.SetCardSpace(m_dwCardHSpace,0,20);

	return true;
}

//重置框架
void CGameClientDlg::ResetGameFrame()
{
	//游戏变量
	m_wBombTime=1;
	m_bHandCardCount=0;
	m_wLandUser=INVALID_CHAIR;
	memset(m_bCardCount,0,sizeof(m_bCardCount));
	memset(m_bHandCardData,0,sizeof(m_bHandCardData));

	//
	m_wXNum = 1;

	//出牌变量
	m_bTurnCardCount=0;
	m_bTurnOutType=CT_INVALID;
	memset(m_bTurnCardData,0,sizeof(m_bTurnCardData));

	//辅助变量
	m_wTimeOutCount=0;
	m_wMostUser=INVALID_CHAIR;

	//删除定时
	KillGameTimer(0);
	KillTimer(IDI_MOST_CARD);

	return;
}

//游戏设置
void CGameClientDlg::OnGameOptionSet()
{
	//构造数据
	CGameOption GameOption;
	GameOption.m_dwCardHSpace=m_dwCardHSpace;
	GameOption.m_bEnableSound=IsEnableSound();
	GameOption.m_bDeasilOrder=m_GameClientView.IsDeasilOrder();

	//配置数据
	if (GameOption.DoModal()==IDOK)
	{
		//获取参数
		m_bDeasilOrder=GameOption.m_bDeasilOrder;
		m_dwCardHSpace=GameOption.m_dwCardHSpace;

		//设置控件
		EnableSound(GameOption.m_bEnableSound);
		m_GameClientView.SetUserOrder(GameOption.m_bDeasilOrder);
		m_GameClientView.m_HandCardControl.SetCardSpace(m_dwCardHSpace,0,20);

		//保存配置
		AfxGetApp()->WriteProfileInt(TEXT("GameOption"),TEXT("CardSpace"),m_dwCardHSpace);
		AfxGetApp()->WriteProfileInt(TEXT("GameOption"),TEXT("DeasilOrder"),m_bDeasilOrder?TRUE:FALSE);
	}

	return;
}

//时间消息
bool CGameClientDlg::OnTimerMessage(WORD wChairID, UINT nElapse, UINT nTimerID)
{
	switch (nTimerID)
	{
	case IDI_OUT_CARD:			//用户出牌
		{
			//超时判断
			if (nElapse==0)
			{
				if ((IsLookonMode()==false)&&(wChairID==GetMeChairID())) AutomatismOutCard();
				return false;
			}

			//播放声音
			if (m_bHandCardCount<m_bTurnCardCount) return true;
			if ((nElapse<=10)&&(wChairID==GetMeChairID())&&(IsLookonMode()==false)) PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_WARN"));

			return true;
		}
	case IDI_START_GAME:		//开始游戏
		{
			if (nElapse==0)
			{
				if ((IsLookonMode()==false)&&(wChairID==GetMeChairID())) OnStart(0,0);
				return false;
			}
			if ((nElapse<=10)&&(wChairID==GetMeChairID())&&(IsLookonMode()==false)) PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_WARN"));

			return true;
		}
	case IDI_LAND_SCORE:		//挖坑叫分
		{
			if (nElapse==0)
			{
				if ((IsLookonMode()==false)&&(wChairID==GetMeChairID())) OnQiang(255,255);
				return false;
			}
			if ((nElapse<=10)&&(wChairID==GetMeChairID())&&(IsLookonMode()==false)) PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_WARN"));

			return true;
		}

	case IDI_QIANG_LAND://抢地主
		{
			if (nElapse==0)
			{
				if ((IsLookonMode()==false)&&(wChairID==GetMeChairID())) OnLandScore(255,255);
				return false;
			}
			if ((nElapse<=10)&&(wChairID==GetMeChairID())&&(IsLookonMode()==false)) PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_WARN"));

			return true;
		}
	}

	return false;
}

//旁观状态
void CGameClientDlg::OnLookonChanged(bool bLookonUser, const void * pBuffer, WORD wDataSize)
{
}

//网络消息
bool CGameClientDlg::OnGameMessage(WORD wSubCmdID, const void * pBuffer, WORD wDataSize)
{
	switch (wSubCmdID)
	{
	case SUB_S_SEND_CARD:		//发送扑克
		{
			return OnSubSendCard(pBuffer,wDataSize);
		}
	case SUB_S_LAND_SCORE:	//用户叫分
		{
			return OnSubLandScore(pBuffer,wDataSize);
		}
	case  SUB_S_QAING_LAND	:				//抢地主命令
		{

			return OnSubQiangLand(pBuffer,wDataSize);

		}

	case SUB_S_GAME_START:		//游戏开始
		{
			return OnSubGameStart(pBuffer,wDataSize);
		}
	case SUB_S_OUT_CARD:		//用户出牌
		{
			return OnSubOutCard(pBuffer,wDataSize);
		}
	case SUB_S_PASS_CARD:		//放弃出牌
		{
			return OnSubPassCard(pBuffer,wDataSize);
		}
	case SUB_S_GAME_END:		//游戏结束
		{
			return OnSubGameEnd(pBuffer,wDataSize);
		}
	}

	return false;
}

//游戏场景
bool CGameClientDlg::OnGameSceneMessage(BYTE cbGameStation, bool bLookonOther, const void * pBuffer, WORD wDataSize)
{
	switch (cbGameStation)
	{
	case GS_WK_FREE:	//空闲状态
		{
			//效验数据
			if (wDataSize!=sizeof(CMD_S_StatusFree)) return false;
			CMD_S_StatusFree * pStatusFree=(CMD_S_StatusFree *)pBuffer;

			//设置界面
			m_GameClientView.SetBaseScore(pStatusFree->lBaseScore);

			//设置控件
			if (IsLookonMode()==false)
			{
				m_GameClientView.m_btStart.ShowWindow(TRUE);
				m_GameClientView.m_btStart.SetFocus();
			}

			//设置扑克
			if (IsLookonMode()==false) m_GameClientView.m_HandCardControl.SetDisplayFlag(true);

			return true;
		}
	case GS_WK_SCORE:	//叫分状态
		{
			//效验数据
			if (wDataSize!=sizeof(CMD_S_StatusScore)) return false;
			CMD_S_StatusScore * pStatusScore=(CMD_S_StatusScore *)pBuffer;

			//设置变量
			m_bHandCardCount=17;
			for (WORD i=0;i<GAME_PLAYER;i++) m_bCardCount[i]=17;
			CopyMemory(m_bHandCardData,pStatusScore->bCardData,m_bHandCardCount);

			//设置界面
			for (WORD i=0;i<GAME_PLAYER;i++)	
			{
				WORD wViewChairID=SwitchViewChairID(i);
				m_GameClientView.SetCardCount(wViewChairID,m_bCardCount[i]);
				m_GameClientView.SetLandScore(wViewChairID,pStatusScore->bScoreInfo[i]);
			}
			m_GameClientView.ShowLandTitle(true);
			m_GameClientView.SetBaseScore(pStatusScore->lBaseScore);

			//按钮控制
			if ((IsLookonMode()==false)&&(pStatusScore->wCurrentUser==GetMeChairID()))
			{
				m_GameClientView.m_btGiveUpScore.ShowWindow(SW_SHOW);
				m_GameClientView.m_btOneScore.ShowWindow(pStatusScore->bLandScore<=0?SW_SHOW:SW_HIDE);
				m_GameClientView.m_btTwoScore.ShowWindow(pStatusScore->bLandScore<=1?SW_SHOW:SW_HIDE);
				m_GameClientView.m_btThreeScore.ShowWindow(pStatusScore->bLandScore<=2?SW_SHOW:SW_HIDE);
			}

			//设置扑克
			BYTE bCardData[3]={0,0,0};
			m_GameClientView.m_BackCardControl.SetCardData(bCardData,3);
			m_GameClientView.m_HandCardControl.SetCardData(m_bHandCardData,m_bHandCardCount);
			if (IsLookonMode()==false) m_GameClientView.m_HandCardControl.SetDisplayFlag(true);

			//设置时间
			SetGameTimer(pStatusScore->wCurrentUser,IDI_LAND_SCORE,30);

			return true;
		}
	case GS_WK_QIANG://抢地主
		{
			return true;
		}
	case GS_WK_PLAYING:		//游戏状态
		{
			//效验数据
			if (wDataSize!=sizeof(CMD_S_StatusPlay)) return false;
			CMD_S_StatusPlay * pStatusPlay=(CMD_S_StatusPlay *)pBuffer;

			//设置变量
			m_bTurnCardCount=pStatusPlay->bTurnCardCount;
			m_bHandCardCount=pStatusPlay->bCardCount[GetMeChairID()];
			m_bTurnOutType=m_GameLogic.GetCardType(pStatusPlay->bTurnCardData,m_bTurnCardCount);
			CopyMemory(m_bHandCardData,pStatusPlay->bCardData,m_bHandCardCount);
			CopyMemory(m_bTurnCardData,pStatusPlay->bTurnCardData,pStatusPlay->bTurnCardCount);

			//设置界面
			for (BYTE i=0;i<GAME_PLAYER;i++)
			{
				WORD wViewChairID=SwitchViewChairID(i);
				m_bCardCount[i]=pStatusPlay->bCardCount[i];
				m_GameClientView.SetCardCount(wViewChairID,pStatusPlay->bCardCount[i]);
			}
			m_GameClientView.SetBombTime(pStatusPlay->wBombTime);
			m_GameClientView.SetBaseScore(pStatusPlay->lBaseScore);
			m_GameClientView.m_BackCardControl.SetCardData(pStatusPlay->bBackCard,3);
			m_GameClientView.m_HandCardControl.SetCardData(m_bHandCardData,m_bHandCardCount);
			m_GameClientView.SetLandUser(SwitchViewChairID(pStatusPlay->wLandUser),pStatusPlay->bLandScore);

			//玩家设置
			if ((IsLookonMode()==false)&&(pStatusPlay->wCurrentUser==GetMeChairID()))
			{
				m_GameClientView.m_btOutCard.EnableWindow(FALSE);
				m_GameClientView.m_btOutCard.ShowWindow(SW_SHOW);
				m_GameClientView.m_btPassCard.ShowWindow(SW_SHOW);
				m_GameClientView.m_btAutoOutCard.ShowWindow(SW_SHOW);
				m_GameClientView.m_btPassCard.EnableWindow((m_bTurnCardCount!=0)?TRUE:FALSE);
				m_GameClientView.m_btAutoOutCard.EnableWindow((m_bTurnCardCount!=0)?TRUE:FALSE);
			}

			//桌面设置
			if (m_bTurnCardCount!=0)
			{
				WORD wViewChairID=SwitchViewChairID(pStatusPlay->wLastOutUser);
				m_GameClientView.m_UserCardControl[wViewChairID].SetCardData(m_bTurnCardData,m_bTurnCardCount);
			}

			//设置定时器
			SetGameTimer(pStatusPlay->wCurrentUser,IDI_OUT_CARD,30);

			//设置扑克
			if (IsLookonMode()==false) 
			{
				m_GameClientView.m_HandCardControl.SetPositively(true);
				m_GameClientView.m_HandCardControl.SetDisplayFlag(true);
			}

			return true;
		}
	}

	return false;
}

//发送扑克
bool CGameClientDlg::OnSubSendCard(const void * pBuffer, WORD wDataSize)
{
	//效验数据
	ASSERT(wDataSize==sizeof(CMD_S_SendCard));
	if (wDataSize!=sizeof(CMD_S_SendCard)) return false;

	//变量定义
	CMD_S_SendCard * pSendCard=(CMD_S_SendCard *)pBuffer;

	//设置数据
	m_bHandCardCount=CountArray(pSendCard->bCardData);
	CopyMemory(m_bHandCardData,pSendCard->bCardData,sizeof(pSendCard->bCardData));
	for (WORD i=0;i<GAME_PLAYER;i++) m_bCardCount[i]=CountArray(pSendCard->bCardData);

	//设置界面
	for (WORD i=0;i<GAME_PLAYER;i++)
	{
		m_GameClientView.SetLandScore(i,0);
		m_GameClientView.SetPassFlag(i,false);
		m_GameClientView.SetCardCount(i,m_bCardCount[i]);
		m_GameClientView.m_UserCardControl[i].SetCardData(NULL,0);
	}
	if (IsLookonMode()==true)
	{
		m_GameClientView.SetLandUser(INVALID_CHAIR,0);
		m_GameClientView.m_ScoreView.ShowWindow(SW_HIDE);
		m_GameClientView.m_LeaveCardControl[0].SetCardData(NULL,0);
		m_GameClientView.m_LeaveCardControl[1].SetCardData(NULL,0);
	}
	m_GameClientView.ShowLandTitle(true);
	m_GameClientView.SetBombTime(m_wBombTime);

	//设置扑克
	BYTE bBackCard[3]={0,0,0};
	m_GameClientView.m_HandCardControl.SetCardData(pSendCard->bCardData,17);
	m_GameClientView.m_BackCardControl.SetCardData(bBackCard,CountArray(bBackCard));
	if (IsLookonMode()==true) m_GameClientView.m_HandCardControl.SetDisplayFlag(false);

	//当前玩家
	if ((IsLookonMode()==false)&&(pSendCard->wCurrentUser==GetMeChairID()))
	{
		ActiveGameFrame();
		m_GameClientView.m_btOneScore.ShowWindow(SW_SHOW);
		m_GameClientView.m_btTwoScore.ShowWindow(SW_SHOW);
		m_GameClientView.m_btThreeScore.ShowWindow(SW_SHOW);
		m_GameClientView.m_btGiveUpScore.ShowWindow(SW_SHOW);
	}

	//播放声音
	PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_START"));

	//设置时间
	SetGameTimer(pSendCard->wCurrentUser,IDI_LAND_SCORE,30);

	return true;
}
//用户抢地主
bool CGameClientDlg::OnSubQiangLand(const void * pBuffer, WORD wDataSize)
{

	//效验数据
	ASSERT(wDataSize==sizeof(CMD_S_QiangLand));
	if (wDataSize!=sizeof(CMD_S_QiangLand)) return false;

	//消息处理
	CMD_S_QiangLand * pLandScore=(CMD_S_QiangLand *)pBuffer;

	//设置界面
	WORD wViewChairID=SwitchViewChairID(pLandScore->bLandUser);
	m_GameClientView.SetQiangLand(wViewChairID,pLandScore->bLandScore);

	//玩家设置
	if ((IsLookonMode()==false)&&(pLandScore->wCurrentUser==GetMeChairID()))
	{
		ActiveGameFrame();

		m_GameClientView.m_btQiang.ShowWindow(SW_SHOW);
		m_GameClientView.m_btNoQiang.ShowWindow(SW_SHOW);
	}

	//播放声音
	PlayGameSound(AfxGetInstanceHandle(),TEXT("OUT_CARD"));

	//设置时间
	SetGameTimer(pLandScore->wCurrentUser,IDI_QIANG_LAND,30);
	return true;

}

//用户叫分
bool CGameClientDlg::OnSubLandScore(const void * pBuffer, WORD wDataSize)
{
	//效验数据
	ASSERT(wDataSize==sizeof(CMD_S_LandScore));
	if (wDataSize!=sizeof(CMD_S_LandScore)) return false;

	//消息处理
	CMD_S_LandScore * pLandScore=(CMD_S_LandScore *)pBuffer;

	//设置界面
	WORD wViewChairID=SwitchViewChairID(pLandScore->bLandUser);
	m_GameClientView.SetLandScore(wViewChairID,pLandScore->bLandScore);

	//玩家设置
	if ((IsLookonMode()==false)&&(pLandScore->wCurrentUser==GetMeChairID()))
	{
		ActiveGameFrame();
		m_GameClientView.m_btGiveUpScore.ShowWindow(SW_SHOW);
		m_GameClientView.m_btOneScore.ShowWindow(pLandScore->bCurrentScore<=0?SW_SHOW:SW_HIDE);
		m_GameClientView.m_btTwoScore.ShowWindow(pLandScore->bCurrentScore<=1?SW_SHOW:SW_HIDE);
		m_GameClientView.m_btThreeScore.ShowWindow(pLandScore->bCurrentScore<=2?SW_SHOW:SW_HIDE);
	}

	//播放声音
	int fenInt=pLandScore->bCurrentScore;
	int sexInt= 0;
	PlayJiaoFenSound(  sexInt,fenInt);
	//	PlayGameSound(AfxGetInstanceHandle(),TEXT("OUT_CARD"));

	//设置时间
	SetGameTimer(pLandScore->wCurrentUser,IDI_LAND_SCORE,30);

	return true;
}

//游戏开始
bool CGameClientDlg::OnSubGameStart(const void * pBuffer, WORD wDataSize)
{
	//效验数据
	ASSERT(wDataSize==sizeof(CMD_S_GameStart));
	if (wDataSize!=sizeof(CMD_S_GameStart)) return false;

	//消息处理
	CMD_S_GameStart * pGameStart=(CMD_S_GameStart *)pBuffer;

	//设置变量
	m_wBombTime=1;
	m_bTurnCardCount=0;
	m_bTurnOutType=CT_INVALID;
	m_wLandUser=pGameStart->wLandUser;

	m_wXNum = pGameStart->m_xNum;//倍数

	m_bCardCount[pGameStart->wLandUser]=20;
	ZeroMemory(m_bTurnCardData,sizeof(m_bTurnCardData));

	//设置控件
	m_GameClientView.ShowLandTitle(false);
	m_GameClientView.m_BackCardControl.SetCardData(pGameStart->bBackCard,CountArray(pGameStart->bBackCard));

	//设置界面
	m_GameClientView.SetLandScore(INVALID_CHAIR,0);
	m_GameClientView.SetCardCount(SwitchViewChairID(pGameStart->wLandUser),m_bCardCount[pGameStart->wLandUser]);

	//地主设置
	if (pGameStart->wLandUser==GetMeChairID())
	{
		BYTE bCardCound=m_bHandCardCount;
		m_bHandCardCount+=CountArray(pGameStart->bBackCard);
		CopyMemory(&m_bHandCardData[bCardCound],pGameStart->bBackCard,sizeof(pGameStart->bBackCard));
		m_GameLogic.SortCardList(m_bHandCardData,m_bHandCardCount);
		m_GameClientView.m_HandCardControl.SetCardData(m_bHandCardData,m_bHandCardCount);
	}
	m_GameClientView.SetLandUser(SwitchViewChairID(pGameStart->wLandUser),pGameStart->bLandScore);

	//SetXNum(WORD wXNum)
	m_GameClientView.SetXNum(m_wXNum);

	//玩家设置
	if (IsLookonMode()==false) m_GameClientView.m_HandCardControl.SetPositively(true);

	//当前玩家
	if ((IsLookonMode()==false)&&(pGameStart->wCurrentUser==GetMeChairID()))
	{
		ActiveGameFrame();
		m_GameClientView.m_btOutCard.EnableWindow(FALSE);
		m_GameClientView.m_btOutCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btPassCard.EnableWindow(FALSE);
		m_GameClientView.m_btPassCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btAutoOutCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btAutoOutCard.EnableWindow(FALSE);
	}

	//播放声音
	PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_START"));

	//设置时间
	SetGameTimer(pGameStart->wCurrentUser,IDI_OUT_CARD,30);

	return true;
}

//用户出牌
bool CGameClientDlg::OnSubOutCard(const void * pBuffer, WORD wDataSize)
{
	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sOnSubOutCard.log",strTime);

	strTemp.Format("OnSubOutCard");
	WriteLog(strFile, strTemp);
	//变量定义
	CMD_S_OutCard * pOutCard=(CMD_S_OutCard *)pBuffer;
	WORD wHeadSize=sizeof(CMD_S_OutCard)-sizeof(pOutCard->bCardData);

	//效验数据
	ASSERT(wDataSize>=wHeadSize);
	if (wDataSize<wHeadSize) return false;
	ASSERT(wDataSize==(wHeadSize+pOutCard->bCardCount*sizeof(pOutCard->bCardData[0])));
	if (wDataSize!=(wHeadSize+pOutCard->bCardCount*sizeof(pOutCard->bCardData[0]))) return false;

	//删除定时器
	KillTimer(IDI_MOST_CARD);
	KillGameTimer(IDI_OUT_CARD);

	//界面设置
	WORD wOutViewChairID=SwitchViewChairID(pOutCard->wOutCardUser);
	m_bCardCount[pOutCard->wOutCardUser]-=pOutCard->bCardCount;
	m_GameClientView.SetCardCount(wOutViewChairID,m_bCardCount[pOutCard->wOutCardUser]);

	//出牌设置
	if ((IsLookonMode()==true)||(GetMeChairID()!=pOutCard->wOutCardUser))
	{
		m_GameClientView.m_UserCardControl[wOutViewChairID].SetCardData(pOutCard->bCardData,pOutCard->bCardCount);
	}

	//清理桌面
	if (m_bTurnCardCount==0)
	{
		for (WORD i=0;i<GAME_PLAYER;i++) 
		{
			if (i==pOutCard->wOutCardUser) continue;
			m_GameClientView.m_UserCardControl[SwitchViewChairID(i)].SetCardData(NULL,0);
		}
		m_GameClientView.SetPassFlag(INVALID_CHAIR,false);
	}

	//记录出牌
	m_bTurnCardCount=pOutCard->bCardCount;
	m_bTurnOutType=m_GameLogic.GetCardType(pOutCard->bCardData,pOutCard->bCardCount);
	CopyMemory(m_bTurnCardData,pOutCard->bCardData,sizeof(BYTE)*pOutCard->bCardCount);

	//动画显示
	PlayCardMovie(m_bTurnOutType);
	//炸弹判断
	if ((m_bTurnOutType==CT_BOMB_CARD)||(m_bTurnOutType==CT_MISSILE_CARD))
	{
		m_wBombTime*=2;
		m_GameClientView.SetBombTime(m_wBombTime);
	}

	//自己扑克
	if ((IsLookonMode()==true)&&(pOutCard->wOutCardUser==GetMeChairID()))
	{
		//删除扑克 
		BYTE bSourceCount=m_bHandCardCount;
		m_bHandCardCount-=pOutCard->bCardCount;
		m_GameLogic.RemoveCard(pOutCard->bCardData,pOutCard->bCardCount,m_bHandCardData,bSourceCount);

		//设置界面
		m_GameClientView.m_HandCardControl.SetCardData(m_bHandCardData,m_bHandCardCount);
	}

	//最大判断
	if (pOutCard->wCurrentUser==pOutCard->wOutCardUser)
	{
		//设置变量
		m_bTurnCardCount=0;
		m_bTurnOutType=CT_INVALID;
		m_wMostUser=pOutCard->wCurrentUser;
		memset(m_bTurnCardData,0,sizeof(m_bTurnCardData));

		//设置界面
		for (WORD i=0;i<GAME_PLAYER;i++)
		{
			if (i!=pOutCard->wOutCardUser)
			{
				WORD wViewChairID=SwitchViewChairID(i);
				m_GameClientView.SetPassFlag(wViewChairID,true);
				m_GameClientView.m_UserCardControl[wViewChairID].SetCardData(NULL,0);
			}
		}

		//播放声音
		PlayGameSound(AfxGetInstanceHandle(),TEXT("MOST_CARD"));

		//设置定时器
		SetTimer(IDI_MOST_CARD,3000,NULL);

		return true;
	}

	//出牌声音
	int cardTypeInt=m_bTurnOutType;// = m_Logic.GetCardShape( pOutCardInfo->iCardList, pOutCardInfo->iCardCount);

	int sexInt ;
	sexInt = 0;// m_pUserInfo[pOutCardInfo->bDeskStation]->GameUserInfo.sexInt;

	if ( cardTypeInt == CT_SINGLE || cardTypeInt == CT_DOUBLE)
	{
		int cardInt = pOutCard->bCardData[0];

		if (cardInt == 0x41 || cardInt ==0x42 )
		{
			PlayOutCardSound(  sexInt,cardTypeInt,cardInt);
			strTemp.Format("王牌!!");
			WriteLog(strFile, strTemp);
		}
		else
		{
			PlayOutCardSound(  sexInt,cardTypeInt, m_GameLogic.GetCardValue(cardInt) -3);
		}

		strTemp.Format("cardTypeInt=%d,cardInt=%d, %d", cardTypeInt,cardInt,m_GameLogic.GetCardValue(cardInt));
		WriteLog(strFile, strTemp);

	}
	else
	{
		PlayOutCardSound( sexInt, cardTypeInt);
	}

	//播放声音
	if ((IsLookonMode()==true)||(GetMeChairID()!=pOutCard->wOutCardUser)) PlayGameSound(AfxGetInstanceHandle(),TEXT("OUT_CARD"));

	//玩家设置
	if (pOutCard->wCurrentUser!=INVALID_CHAIR)
	{
		WORD wViewChairID=SwitchViewChairID(pOutCard->wCurrentUser);
		m_GameClientView.SetPassFlag(wViewChairID,false);
		m_GameClientView.m_UserCardControl[wViewChairID].SetCardData(NULL,0);
	}

	//玩家设置
	if ((IsLookonMode()==false)&&(pOutCard->wCurrentUser==GetMeChairID()))
	{
		ActiveGameFrame();
		m_GameClientView.m_btPassCard.EnableWindow(TRUE);
		m_GameClientView.m_btOutCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btPassCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btAutoOutCard.EnableWindow(TRUE);
		m_GameClientView.m_btAutoOutCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btOutCard.EnableWindow((VerdictOutCard()==true)?TRUE:FALSE);
	}

	//设置时间
	if (pOutCard->wCurrentUser!=INVALID_CHAIR)
	{
		BYTE bCardCount=m_bCardCount[pOutCard->wCurrentUser];
		SetGameTimer(pOutCard->wCurrentUser,IDI_OUT_CARD,(bCardCount<m_bTurnCardCount)?3:30);
	}

	return true;
}

//放弃出牌
bool CGameClientDlg::OnSubPassCard(const void * pBuffer, WORD wDataSize)
{
	//效验数据
	if (wDataSize!=sizeof(CMD_S_PassCard)) return false;
	CMD_S_PassCard * pPassCard=(CMD_S_PassCard *)pBuffer;
	//播放声音
	int sexInt=0;

	//	sexInt = m_pUserInfo[m_iNowOutPeople]->GameUserInfo.sexInt;

	PlayPassSound(sexInt);

	//删除定时器
	KillGameTimer(IDI_OUT_CARD);

	//玩家设置
	if ((IsLookonMode()==true)||(pPassCard->wPassUser!=GetMeChairID()))
	{
		WORD wViewChairID=SwitchViewChairID(pPassCard->wPassUser);
		m_GameClientView.SetPassFlag(wViewChairID,true);
		m_GameClientView.m_UserCardControl[wViewChairID].SetCardData(NULL,0);
	}

	//一轮判断
	if (pPassCard->bNewTurn==TRUE)
	{
		m_bTurnCardCount=0;
		m_bTurnOutType=CT_INVALID;
		memset(m_bTurnCardData,0,sizeof(m_bTurnCardData));
	}

	//设置界面
	WORD wViewChairID=SwitchViewChairID(pPassCard->wCurrentUser);
	m_GameClientView.SetPassFlag(wViewChairID,false);
	m_GameClientView.m_UserCardControl[wViewChairID].SetCardData(NULL,0);

	//玩家设置
	if ((IsLookonMode()==false)&&(pPassCard->wCurrentUser==GetMeChairID()))
	{
		ActiveGameFrame();
		m_GameClientView.m_btOutCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btPassCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btAutoOutCard.ShowWindow(SW_SHOW);
		m_GameClientView.m_btPassCard.EnableWindow((m_bTurnCardCount>0)?TRUE:FALSE);
		m_GameClientView.m_btOutCard.EnableWindow((VerdictOutCard()==true)?TRUE:FALSE);
		m_GameClientView.m_btAutoOutCard.EnableWindow((m_bTurnCardCount>0)?TRUE:FALSE);
	}

	//播放声音
	if ((IsLookonMode()==true)||(pPassCard->wPassUser!=GetMeChairID()))	PlayGameSound(AfxGetInstanceHandle(),TEXT("OUT_CARD"));

	//设置时间
	if (m_bTurnCardCount!=0)
	{
		BYTE bCardCount=m_bCardCount[pPassCard->wCurrentUser];
		SetGameTimer(pPassCard->wCurrentUser,IDI_OUT_CARD,(bCardCount<m_bTurnCardCount)?3:30);
	}
	else SetGameTimer(pPassCard->wCurrentUser,IDI_OUT_CARD,30);

	return true;
}

//游戏结束
bool CGameClientDlg::OnSubGameEnd(const void * pBuffer, WORD wDataSize)
{
	//效验数据
	ASSERT(wDataSize==sizeof(CMD_S_GameEnd));
	if (wDataSize!=sizeof(CMD_S_GameEnd)) return false;

	//消息处理
	CMD_S_GameEnd * pGameEnd=(CMD_S_GameEnd *)pBuffer;

	//删除定时器
	KillTimer(IDI_MOST_CARD);
	KillGameTimer(IDI_OUT_CARD);
	KillGameTimer(IDI_LAND_SCORE);

	//隐藏控件
	m_GameClientView.m_btOutCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btPassCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btOneScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btTwoScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btThreeScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btGiveUpScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btAutoOutCard.ShowWindow(SW_HIDE);

	//禁用控件
	m_GameClientView.m_btOutCard.EnableWindow(FALSE);
	m_GameClientView.m_btPassCard.EnableWindow(FALSE);

	//设置积分
	for (WORD i=0;i<GAME_PLAYER;i++)
	{
		const tagUserData * pUserData=GetUserData(i);
		m_GameClientView.m_ScoreView.SetGameScore(i,pUserData->szName,pGameEnd->lGameScore[i]);
	}
	m_GameClientView.m_ScoreView.SetGameTax(pGameEnd->lGameTax);
	m_GameClientView.m_ScoreView.ShowWindow(SW_SHOW);

	//设置扑克
	BYTE bCardPos=0;
	for (WORD i=0;i<GAME_PLAYER;i++)
	{
		WORD wViewChairID=SwitchViewChairID(i);
		if (wViewChairID==0) m_GameClientView.m_LeaveCardControl[0].SetCardData(&pGameEnd->bCardData[bCardPos],pGameEnd->bCardCount[i]);
		else if (wViewChairID==2) m_GameClientView.m_LeaveCardControl[1].SetCardData(&pGameEnd->bCardData[bCardPos],pGameEnd->bCardCount[i]);
		bCardPos+=pGameEnd->bCardCount[i];
		if (pGameEnd->bCardCount[i]!=0)
		{
			m_GameClientView.SetPassFlag(wViewChairID,false);
			m_GameClientView.m_UserCardControl[wViewChairID].SetCardData(NULL,0);
		}
	}

	//显示扑克
	if (IsLookonMode()==true) m_GameClientView.m_HandCardControl.SetDisplayFlag(true);

	//播放声音
	WORD wMeChairID=GetMeChairID();
	LONG lMeScore=pGameEnd->lGameScore[GetMeChairID()];
	if (lMeScore>0L) PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_WIN"));
	else if (lMeScore<0L) PlayGameSound(AfxGetInstanceHandle(),TEXT("GAME_LOST"));
	else PlayGameSound(GetModuleHandle(NULL),TEXT("GAME_END"));

	//设置界面
	if (IsLookonMode()==false)
	{
		m_GameClientView.m_btStart.ShowWindow(SW_SHOW);
		SetGameTimer(GetMeChairID(),IDI_START_GAME,90);
	}
	m_GameClientView.ShowLandTitle(false);

	return true;
}

//出牌判断
bool CGameClientDlg::VerdictOutCard()
{
	//状态判断
	if (m_GameClientView.m_btOutCard.IsWindowVisible()==FALSE) return false;

	//获取扑克
	BYTE bCardData[20];
	BYTE bShootCount=(BYTE)m_GameClientView.m_HandCardControl.GetShootCard(bCardData,CountArray(bCardData));

	//出牌判断
	if (bShootCount>0L)
	{
		//分析类型
		BYTE bCardType=m_GameLogic.GetCardType(bCardData,bShootCount);

		//类型判断
		if (bCardType==CT_INVALID) return false;

		//跟牌判断
		if (m_bTurnCardCount==0) return true;
		return m_GameLogic.CompareCard(bCardData,m_bTurnCardData,bShootCount,m_bTurnCardCount);
	}

	return false;
}

//自动出牌
bool CGameClientDlg::AutomatismOutCard()
{
	//先出牌者
	if (m_bTurnCardCount==0)
	{
		//控制界面
		KillGameTimer(IDI_OUT_CARD);
		m_GameClientView.m_btOutCard.ShowWindow(SW_HIDE);
		m_GameClientView.m_btOutCard.EnableWindow(FALSE);
		m_GameClientView.m_btPassCard.ShowWindow(SW_HIDE);
		m_GameClientView.m_btPassCard.EnableWindow(FALSE);
		m_GameClientView.m_btAutoOutCard.ShowWindow(SW_HIDE);
		m_GameClientView.m_btAutoOutCard.EnableWindow(FALSE);

		//发送数据
		CMD_C_OutCard OutCard;
		OutCard.bCardCount=1;
		OutCard.bCardData[0]=m_bHandCardData[m_bHandCardCount-1];
		SendData(SUB_C_OUT_CART,&OutCard,sizeof(OutCard)-sizeof(OutCard.bCardData)+OutCard.bCardCount*sizeof(BYTE));

		//预先处理
		PlayGameSound(AfxGetInstanceHandle(),TEXT("OUT_CARD"));
		m_GameClientView.m_UserCardControl[1].SetCardData(OutCard.bCardData,OutCard.bCardCount);

		//预先删除
		BYTE bSourceCount=m_bHandCardCount;
		m_bHandCardCount-=OutCard.bCardCount;
		m_GameLogic.RemoveCard(OutCard.bCardData,OutCard.bCardCount,m_bHandCardData,bSourceCount);
		m_GameClientView.m_HandCardControl.SetCardData(m_bHandCardData,m_bHandCardCount);
	}
	else OnPassCard(0,0);

	return true;
}

//定时器消息
void CGameClientDlg::OnTimer(UINT nIDEvent)
{
	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sOnTimer.log",strTime);

	if( ID_SHOW_PLANE	== nIDEvent)//			109					//飞机
	{
		if(m_GameClientView.myMoiveList[1].currentInt>= m_GameClientView.myMoiveList[1].maxInt-1 )
		{
			m_GameClientView.myMoiveList[1].currentInt = 0;
			strTemp.Format("KillTimer(ID_SHOW_BOMB 1)");
			WriteLog(strFile, strTemp);
			//KillTimer(ID_SHOW_BOMB);
		}
		if ( m_GameClientView.myMoiveList[1].myPoint.x < -m_GameClientView.myMoiveList[1].picW )
		{
			m_GameClientView.myMoiveList[1].useStatus = 0;
			strTemp.Format("KillTimer(ID_SHOW_PLANE 2)");
			WriteLog(strFile, strTemp);
			KillTimer(ID_SHOW_PLANE);
		}
		int l,t,r,b;
		l = m_GameClientView.myMoiveList[1].myPoint.x+m_GameClientView.myMoiveList[1].picW;
		t =m_GameClientView.myMoiveList[1].myPoint.y;
		r=l+m_GameClientView.myMoiveList[1].picW;
		b=t+m_GameClientView.myMoiveList[1].picH;
		CRect myRect(l,
			t,
			r ,
			b
			);
		m_GameClientView.UpdateGameView(NULL);
		m_GameClientView.myMoiveList[1].currentInt++;
		m_GameClientView.myMoiveList[1].myPoint.x += m_GameClientView.myMoiveList[1].addX;

		strTemp.Format("currentInt=%d", m_GameClientView.myMoiveList[1].currentInt);
		WriteLog(strFile, strTemp);


	}
	if( ID_SHOW_ROCKET	== nIDEvent)//			110					//火箭
	{
		if(m_GameClientView.myMoiveList[2].currentInt>= m_GameClientView.myMoiveList[2].maxInt-1)
		{
			m_GameClientView.myMoiveList[2].currentInt = 0;
			strTemp.Format("KillTimer(ID_SHOW_ROCKET)");
			WriteLog(strFile, strTemp);
			//KillTimer(ID_SHOW_BOMB);
		}
		if ( m_GameClientView.myMoiveList[2].myPoint.y < -m_GameClientView.myMoiveList[2].picH  )
		{
			m_GameClientView.myMoiveList[2].useStatus = 0;
			strTemp.Format("KillTimer(ID_SHOW_ROCKET)");
			WriteLog(strFile, strTemp);
			KillTimer(ID_SHOW_ROCKET);
		}
		m_GameClientView.UpdateGameView(NULL);
		m_GameClientView.myMoiveList[2].currentInt++;
		m_GameClientView.myMoiveList[2].myPoint.y -= 30;

	}


	if( ID_SHOW_BOMB == nIDEvent)
	{


		if(m_GameClientView.myMoiveList[0].currentInt>=m_GameClientView.myMoiveList[0].maxInt-1)
		{
			m_GameClientView.myMoiveList[0].useStatus = 0;
			strTemp.Format("KillTimer(ID_SHOW_BOMB)");
			WriteLog(strFile, strTemp);
			KillTimer(ID_SHOW_BOMB);
		}
		m_GameClientView.UpdateGameView(NULL);
		m_GameClientView.myMoiveList[0].currentInt++;
		strTemp.Format("z %d", m_GameClientView.myMoiveList[0].currentInt);
		WriteLog(strFile, strTemp);

	}
	if ( nIDEvent == 1234)
	{
		//设置自己的视频显示窗口
		int x,y,w,h;
		x= 20;//
		y= GetSystemMetrics(SM_CYSCREEN)-123;//rect.Height();//比较难确定 
		w= 85;
		h= 65;
		m_showSelfVideo.MoveWindow( x, y, w, h);
		//	if ( canShowVideo==0)
		m_showSelfVideo.ShowWindow(true);
		//SetupVideoDLL();
		//SetTimer( 1234, 3000, NULL);
		//		ResetVideoWindowPostion();
		//		canShowVideo=1;
		KillTimer( 1234);
	}
	if ((nIDEvent==IDI_MOST_CARD)&&(m_wMostUser!=INVALID_CHAIR))
	{
		//变量定义
		WORD wCurrentUser=m_wMostUser;
		m_wMostUser=INVALID_CHAIR;

		//删除定时器
		KillTimer(IDI_MOST_CARD);

		//设置界面
		m_GameClientView.SetPassFlag(INVALID_CHAIR,false);
		for (WORD i=0;i<GAME_PLAYER;i++) m_GameClientView.m_UserCardControl[i].SetCardData(NULL,0);

		//玩家设置
		if ((IsLookonMode()==false)&&(wCurrentUser==GetMeChairID()))
		{
			ActiveGameFrame();
			m_GameClientView.m_btOutCard.ShowWindow(SW_SHOW);
			m_GameClientView.m_btPassCard.ShowWindow(SW_SHOW);
			m_GameClientView.m_btPassCard.EnableWindow(FALSE);
			m_GameClientView.m_btAutoOutCard.ShowWindow(SW_SHOW);
			m_GameClientView.m_btAutoOutCard.EnableWindow(FALSE);
			m_GameClientView.m_btOutCard.EnableWindow((VerdictOutCard()==true)?TRUE:FALSE);
		}

		//设置时间
		SetGameTimer(wCurrentUser,IDI_OUT_CARD,30);

		return;
	}

	//	__super::OnTimer(nIDEvent);
}
//抢地主按钮
LRESULT CGameClientDlg::OnQiang(WPARAM wParam, LPARAM lParam)
{
	//设置界面
	KillGameTimer(IDI_QIANG_LAND);
	/*
	m_GameClientView.m_btOneScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btTwoScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btThreeScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btGiveUpScore.ShowWindow(SW_HIDE);
	*/

	//发送数据
	CMD_C_QiangLand LandScore;
	LandScore.bLandScore=(BYTE)wParam;
	SendData(SUB_C_QIANG_LAND,&LandScore,sizeof(LandScore));

	return 0;
}

//不抢地主按钮
LRESULT CGameClientDlg::OnNoQiang(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

//开始按钮
LRESULT CGameClientDlg::OnStart(WPARAM wParam, LPARAM lParam)
{
	//设置变量
	m_wBombTime=1;
	m_wTimeOutCount=0;
	m_bHandCardCount=0;
	m_bTurnCardCount=0;
	m_bTurnOutType=CT_INVALID;
	m_wMostUser=INVALID_CHAIR;
	memset(m_bHandCardData,0,sizeof(m_bHandCardData));
	memset(m_bTurnCardData,0,sizeof(m_bTurnCardData));

	//设置界面
	KillGameTimer(IDI_START_GAME);
	m_GameClientView.SetBaseScore(0L);
	m_GameClientView.ShowLandTitle(false);
	m_GameClientView.SetBombTime(m_wBombTime);
	m_GameClientView.SetCardCount(INVALID_CHAIR,0);
	m_GameClientView.SetLandUser(INVALID_CHAIR,0);

	m_GameClientView.SetXNum(1);

	m_GameClientView.SetLandScore(INVALID_CHAIR,0);
	m_GameClientView.SetPassFlag(INVALID_CHAIR,false);

	//隐藏控件
	m_GameClientView.m_btStart.ShowWindow(FALSE);
	m_GameClientView.m_ScoreView.ShowWindow(SW_HIDE);


	//设置扑克
	m_GameClientView.m_BackCardControl.SetCardData(NULL,0);
	m_GameClientView.m_HandCardControl.SetCardData(NULL,0);
	m_GameClientView.m_HandCardControl.SetPositively(false);
	m_GameClientView.m_LeaveCardControl[0].SetCardData(NULL,0);
	m_GameClientView.m_LeaveCardControl[1].SetCardData(NULL,0);
	for (WORD i=0;i<GAME_PLAYER;i++) m_GameClientView.m_UserCardControl[i].SetCardData(NULL,0);

	//发送消息
	SendUserReady(NULL,0);

	return 0;
}

//出牌消息
LRESULT CGameClientDlg::OnOutCard(WPARAM wParam, LPARAM lParam)
{
	//状态判断
	if ((m_GameClientView.m_btOutCard.IsWindowEnabled()==FALSE)||
		(m_GameClientView.m_btOutCard.IsWindowVisible()==FALSE)) return 0;

	//设置界面
	KillGameTimer(IDI_OUT_CARD);
	m_GameClientView.m_btOutCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btOutCard.EnableWindow(FALSE);
	m_GameClientView.m_btPassCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btPassCard.EnableWindow(FALSE);
	m_GameClientView.m_btAutoOutCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btAutoOutCard.EnableWindow(FALSE);

	//发送数据
	CMD_C_OutCard OutCard;
	OutCard.bCardCount=(BYTE)m_GameClientView.m_HandCardControl.GetShootCard(OutCard.bCardData,CountArray(OutCard.bCardData));
	SendData(SUB_C_OUT_CART,&OutCard,sizeof(OutCard)-sizeof(OutCard.bCardData)+OutCard.bCardCount*sizeof(BYTE));

	//预先显示
	PlayGameSound(AfxGetInstanceHandle(),TEXT("OUT_CARD"));
	m_GameClientView.m_UserCardControl[1].SetCardData(OutCard.bCardData,OutCard.bCardCount);

	//预先删除
	BYTE bSourceCount=m_bHandCardCount;
	m_bHandCardCount-=OutCard.bCardCount;
	m_GameLogic.RemoveCard(OutCard.bCardData,OutCard.bCardCount,m_bHandCardData,bSourceCount);
	m_GameClientView.m_HandCardControl.SetCardData(m_bHandCardData,m_bHandCardCount);

	return 0;
}

//放弃出牌
LRESULT CGameClientDlg::OnPassCard(WPARAM wParam, LPARAM lParam)
{
	//状态判断
	if (m_GameClientView.m_btPassCard.IsWindowEnabled()==FALSE) return 0;

	//设置界面
	KillGameTimer(IDI_OUT_CARD);
	m_GameClientView.m_btOutCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btOutCard.EnableWindow(FALSE);
	m_GameClientView.m_btPassCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btPassCard.EnableWindow(FALSE);
	m_GameClientView.m_btAutoOutCard.ShowWindow(SW_HIDE);
	m_GameClientView.m_btAutoOutCard.EnableWindow(FALSE);

	//发送数据
	SendData(SUB_C_PASS_CARD);

	//预先显示
	m_GameClientView.SetPassFlag(1,true);
	PlayGameSound(AfxGetInstanceHandle(),TEXT("OUT_CARD"));
	m_GameClientView.m_HandCardControl.SetCardData(m_bHandCardData,m_bHandCardCount);

	return 0;
}
//抢地主消息	
LRESULT CGameClientDlg::OnQiangLand(WPARAM wParam, LPARAM lParam)
{
	//设置界面
	KillGameTimer(IDI_QIANG_LAND);
	m_GameClientView.m_btQiang.ShowWindow(SW_HIDE);
	m_GameClientView.m_btNoQiang.ShowWindow(SW_HIDE);

	//发送数据
	CMD_C_QiangLand LandScore;
	LandScore.bLandScore=(BYTE)wParam;
	SendData(SUB_C_QIANG_LAND,&LandScore,sizeof(LandScore));

	return 0;
}
//叫分消息
LRESULT CGameClientDlg::OnLandScore(WPARAM wParam, LPARAM lParam)
{
	//设置界面
	KillGameTimer(IDI_LAND_SCORE);
	m_GameClientView.m_btOneScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btTwoScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btThreeScore.ShowWindow(SW_HIDE);
	m_GameClientView.m_btGiveUpScore.ShowWindow(SW_HIDE);

	//发送数据
	CMD_C_LandScore LandScore;
	LandScore.bLandScore=(BYTE)wParam;
	SendData(SUB_C_LAND_SCORE,&LandScore,sizeof(LandScore));

	return 0;
}

//出牌提示
LRESULT CGameClientDlg::OnAutoOutCard(WPARAM wParam, LPARAM lParam)
{
	AutoOutCard(0);
	return 0;
}

//右键扑克
LRESULT CGameClientDlg::OnLeftHitCard(WPARAM wParam, LPARAM lParam)
{
	//设置控件
	bool bOutCard=VerdictOutCard();
	m_GameClientView.m_btOutCard.EnableWindow(bOutCard?TRUE:FALSE);

	return 0;
}

//左键扑克
LRESULT CGameClientDlg::OnRightHitCard(WPARAM wParam, LPARAM lParam)
{
	//用户出牌
	OnOutCard(0,0);

	return 0;
}

void CGameClientDlg::AutoOutCard(BYTE m_WhichOnsKindCard)
{
	int i=0;
	BYTE m_bWhichKindSel=0;
	BYTE							m_bTempSCardCount=0;				//扑克数目
	BYTE							m_bTempSCardData[20];				//手上扑克
	BYTE							m_bTempDCardCount=0;				//扑克数目
	BYTE							m_bTempDCardData[20];				//手上扑克
	BYTE							m_bTempTCardCount=0;				//扑克数目
	BYTE							m_bTempTCardData[20];				//手上扑克
	BYTE							m_bTempFCardCount=0;				//扑克数目
	BYTE							m_bTempFCardData[20];				//手上扑克
	BYTE							m_bTempGetCardCount=0;				//扑克数目
	BYTE							m_bTempGetCardData[20];				//手上扑克
	BYTE							m_bTempGetCardIndex[20];			//手上扑克
	BYTE m_TempCard=0;
	//如果没有人出牌，不提示
	if(m_bTurnCardCount==0)
		return;
	m_GameClientView.m_HandCardControl.ShootAllCard(false);
	//获取单牌列表
	for(i=0;i<m_bHandCardCount;i++)
	{	
		BYTE m_GetCard=m_GameLogic.GetCardLogicValue(m_bHandCardData[i]);
		if(m_TempCard!=m_GetCard)
		{
			m_bTempSCardData[m_bTempSCardCount++]=m_bHandCardData[i];
			m_TempCard=m_GetCard;
		}
	}
	//获取对牌列表
	m_TempCard=0;
	for(i=0;i<m_bHandCardCount-1;i++)
	{	
		BYTE m_GetCard1=m_GameLogic.GetCardLogicValue(m_bHandCardData[i]);
		BYTE m_GetCard2=m_GameLogic.GetCardLogicValue(m_bHandCardData[i+1]);
		if(m_TempCard!=m_GetCard1&&m_GetCard1==m_GetCard2&&m_GetCard1<16)
		{
			m_bTempDCardData[m_bTempDCardCount++]=m_bHandCardData[i];
			m_bTempDCardData[m_bTempDCardCount++]=m_bHandCardData[i+1];
			m_TempCard=m_GetCard1;
		}
	}
	//获取三张牌列表
	m_TempCard=0;
	for(i=0;i<m_bHandCardCount-2;i++)
	{	
		BYTE m_GetCard1=m_GameLogic.GetCardLogicValue(m_bHandCardData[i]);
		BYTE m_GetCard2=m_GameLogic.GetCardLogicValue(m_bHandCardData[i+1]);
		BYTE m_GetCard3=m_GameLogic.GetCardLogicValue(m_bHandCardData[i+2]);
		if(m_TempCard!=m_GetCard1&&m_GetCard1==m_GetCard2&&m_GetCard1==m_GetCard3)
		{
			m_bTempTCardData[m_bTempTCardCount++]=m_bHandCardData[i];
			m_bTempTCardData[m_bTempTCardCount++]=m_bHandCardData[i+1];
			m_bTempTCardData[m_bTempTCardCount++]=m_bHandCardData[i+2];
			m_TempCard=m_GetCard1;
		}
	}
	//获取四张牌列表
	m_TempCard=0;
	for(i=0;i<m_bHandCardCount-3;i++)
	{	
		BYTE m_GetCard1=m_GameLogic.GetCardLogicValue(m_bHandCardData[i]);
		BYTE m_GetCard2=m_GameLogic.GetCardLogicValue(m_bHandCardData[i+1]);
		BYTE m_GetCard3=m_GameLogic.GetCardLogicValue(m_bHandCardData[i+2]);
		BYTE m_GetCard4=m_GameLogic.GetCardLogicValue(m_bHandCardData[i+3]);
		if(m_TempCard!=m_GetCard1&&m_GetCard1==m_GetCard2&&m_GetCard1==m_GetCard3&&m_GetCard1==m_GetCard4)
		{
			m_bTempFCardData[m_bTempFCardCount++]=m_bHandCardData[i];
			m_bTempFCardData[m_bTempFCardCount++]=m_bHandCardData[i+1];
			m_bTempFCardData[m_bTempFCardCount++]=m_bHandCardData[i+2];
			m_bTempFCardData[m_bTempFCardCount++]=m_bHandCardData[i+3];
			m_TempCard=m_GetCard1;
		}
	}
	//根据所出牌类型判断
	i=0;
	switch(m_bTurnOutType)
	{
	case CT_SINGLE:
	case CT_ONE_LINE:
		if(m_WhichOnsKindCard==1)   //判断是不是具有唯一性
		{
			for(i=m_bTempSCardCount;i>0;i--)
			{
				if(i-m_bTurnCardCount>=0&&m_GameLogic.CompareCard(&m_bTempSCardData[i-m_bTurnCardCount],m_bTurnCardData,m_bTurnCardCount,m_bTurnCardCount))
				{
					if((m_bWhichKindSel++)>1)
						i=0;
				}
			}
		}
		for(i=m_bTempSCardCount;i>0;i--)
		{
			if(i-m_bTurnCardCount>=0&&m_GameLogic.CompareCard(&m_bTempSCardData[i-m_bTurnCardCount],m_bTurnCardData,m_bTurnCardCount,m_bTurnCardCount))
			{
				//判断是不是最合理的
				bool m_bIsHaveCard=false;
				for(int j=0;j<m_bTempDCardCount;j++)
				{
					for(int n=0;n<m_bTurnCardCount;n++)
					{
						if(m_GameLogic.GetCardLogicValue(m_bTempSCardData[i-m_bTurnCardCount+n])==m_GameLogic.GetCardLogicValue(m_bTempDCardData[j]))
							m_bIsHaveCard=true;
					}
				}
				//把最合理的情况保存起来
				if(m_bTempGetCardCount==0||!m_bIsHaveCard)
				{
					CopyMemory(m_bTempGetCardData,&m_bTempSCardData[i-m_bTurnCardCount],m_bTurnCardCount);
					m_bTempGetCardCount=m_bTurnCardCount;
				}
				if(!m_bIsHaveCard)
					break;
			}
		}
		break;
	case CT_DOUBLE:
	case CT_DOUBLE_LINE:
		if(m_WhichOnsKindCard==1)     //判断是不是具有唯一性
		{
			for(i=m_bTempDCardCount;i>0;i--)
			{
				if(i-m_bTurnCardCount>=0&&m_GameLogic.CompareCard(&m_bTempDCardData[i-m_bTurnCardCount],m_bTurnCardData,m_bTurnCardCount,m_bTurnCardCount))
				{
					if((m_bWhichKindSel++)>1)
						i=0;
				}
			}
		}
		for(i=m_bTempDCardCount;i>0;i--)
		{
			if(i-m_bTurnCardCount>=0&&m_GameLogic.CompareCard(&m_bTempDCardData[i-m_bTurnCardCount],m_bTurnCardData,m_bTurnCardCount,m_bTurnCardCount))
			{
				//判断是不是最合理的
				bool m_bIsHaveCard=false;
				for(int j=0;j<m_bTempTCardCount;j++)
				{
					for(int n=0;n<m_bTurnCardCount;n++)
					{
						if(m_GameLogic.GetCardLogicValue(m_bTempDCardData[i-m_bTurnCardCount+n])==m_GameLogic.GetCardLogicValue(m_bTempTCardData[j]))
							m_bIsHaveCard=true;
					}
				}
				if(m_bTempGetCardCount==0||!m_bIsHaveCard)
				{
					CopyMemory(m_bTempGetCardData,&m_bTempDCardData[i-m_bTurnCardCount],m_bTurnCardCount);
					m_bTempGetCardCount=m_bTurnCardCount;
				}
				if(!m_bIsHaveCard)
					break;
			}
		}
		break;
	case CT_THREE:
	case CT_THREE_LINE:
		if(m_WhichOnsKindCard==1)           //判断是不是具有唯一性
		{
			for(i=m_bTempTCardCount;i>0;i--)
			{
				if(i-m_bTurnCardCount>=0&&m_GameLogic.CompareCard(&m_bTempTCardData[i-m_bTurnCardCount],m_bTurnCardData,m_bTurnCardCount,m_bTurnCardCount))
				{
					if((m_bWhichKindSel++)>1)
						i=0;
				}
			}
		}
		for(i=m_bTempTCardCount;i>0;i--)
		{
			if(i-m_bTurnCardCount>=0&&m_GameLogic.CompareCard(&m_bTempTCardData[i-m_bTurnCardCount],m_bTurnCardData,m_bTurnCardCount,m_bTurnCardCount))
			{
				//判断是不是最合理的
				bool m_bIsHaveCard=false;
				for(int j=0;j<m_bTempFCardCount;j++)
				{
					for(int n=0;n<m_bTurnCardCount;n++)
					{
						if(m_GameLogic.GetCardLogicValue(m_bTempTCardData[i-m_bTurnCardCount+n])==m_GameLogic.GetCardLogicValue(m_bTempFCardData[j]))
							m_bIsHaveCard=true;
					}
				}
				if(m_bTempGetCardCount==0||!m_bIsHaveCard)
				{
					CopyMemory(m_bTempGetCardData,&m_bTempTCardData[i-m_bTurnCardCount],m_bTurnCardCount);
					m_bTempGetCardCount=m_bTurnCardCount;
				}
				if(!m_bIsHaveCard&&m_bTempGetCardCount!=0)
					break;
			}
		}
		break;
	case CT_THREE_LINE_TAKE_ONE:
	case CT_THREE_LINE_TAKE_DOUBLE:
		{
			//分析扑克
			tagAnalyseResult AnalyseResult;
			m_GameLogic.AnalysebCardData(m_bTurnCardData,m_bTurnCardCount,AnalyseResult);      
			if(m_WhichOnsKindCard==1)               //判断是不是具有唯一性
			{
				for(i=m_bTempTCardCount;i>0;i--)
				{
					if(i-AnalyseResult.bThreeCount*3>=0&&m_GameLogic.CompareCard(&m_bTempTCardData[i-AnalyseResult.bThreeCount*3],m_bTurnCardData,AnalyseResult.bThreeCount*3,AnalyseResult.bThreeCount*3))
					{
						if((m_bWhichKindSel++)>1)
							i=0;
					}
				}
			}
			for(i=m_bTempTCardCount;i>0;i--)
			{
				if(i-AnalyseResult.bThreeCount*3>=0&&m_GameLogic.CompareCard(&m_bTempTCardData[i-AnalyseResult.bThreeCount*3],AnalyseResult.m_bTCardData,AnalyseResult.bThreeCount*3,AnalyseResult.bThreeCount*3))
				{
					//判断是不是最合理的
					bool m_bIsHaveCard=false;
					for(int j=0;j<m_bTempFCardCount;j++)
					{
						for(int n=0;n<AnalyseResult.bThreeCount*3;n++)
						{
							if(m_GameLogic.GetCardLogicValue(m_bTempTCardData[i-AnalyseResult.bThreeCount*3+n])==m_GameLogic.GetCardLogicValue(m_bTempFCardData[j]))
								m_bIsHaveCard=true;
						}
					}
					if(m_bTempGetCardCount==0||!m_bIsHaveCard)
					{
						CopyMemory(m_bTempGetCardData,&m_bTempTCardData[i-AnalyseResult.bThreeCount*3],AnalyseResult.bThreeCount*3);
						m_bTempGetCardCount=AnalyseResult.bThreeCount*3;
					}
					if(!m_bIsHaveCard&&m_bTempGetCardCount!=0)
						i=0;
				}
			}
			if(m_bTempGetCardCount>0)
			{
				bool m_bIsHaveSame;
				for(int m=0;m<AnalyseResult.bDoubleCount;m++)
				{
					for(int j=0;j<m_bTempDCardCount/2;j++)
					{
						//判断是不是最合理的
						m_bIsHaveSame=false;
						for(int n=0;n<m_bTempGetCardCount;n++)
						{
							if(m_GameLogic.GetCardLogicValue(m_bTempDCardData[m_bTempDCardCount-j*2-1])==m_GameLogic.GetCardLogicValue(m_bTempGetCardData[n]))
							{
								m_bIsHaveSame=true;
							}
						}
						if(!m_bIsHaveSame)
						{
							bool m_bIsHaveCard=false;
							for(int s=0;s<m_bTempTCardCount;s++)
							{
								for(int n=0;n<m_bTempGetCardCount;n++)
								{
									if(m_GameLogic.GetCardLogicValue(m_bTempDCardData[m_bTempDCardCount-j*2-1])==m_GameLogic.GetCardLogicValue(m_bTempTCardData[s]))
										m_bIsHaveCard=true;
								}
							}
							if(m_bTempGetCardCount==AnalyseResult.bThreeCount*3||!m_bIsHaveCard)
							{
								m_bTempGetCardData[AnalyseResult.bThreeCount*3+m*2]=m_bTempDCardData[m_bTempDCardCount-j*2-1];
								m_bTempGetCardData[AnalyseResult.bThreeCount*3+m*2+1]=m_bTempDCardData[m_bTempDCardCount-j*2-2];
								m_bTempGetCardCount=AnalyseResult.bThreeCount*3+(m+1)*2;
							}
							if(!m_bIsHaveCard)
							{
								n=m_bTempGetCardCount;
								j=m_bTempDCardCount/2;
							}
						}
					}
				}
				for(int m=0;m<AnalyseResult.bSignedCount;m++)
				{
					for(int j=0;j<m_bTempSCardCount;j++)
					{
						//判断是不是最合理的
						m_bIsHaveSame=false;
						for(int n=0;n<m_bTempGetCardCount;n++)
						{
							if(m_GameLogic.GetCardLogicValue(m_bTempSCardData[m_bTempSCardCount-j-1])==m_GameLogic.GetCardLogicValue(m_bTempGetCardData[n]))
							{
								m_bIsHaveSame=true;
							}
						}
						if(!m_bIsHaveSame)
						{
							bool m_bIsHaveCard=false;
							for(int s=0;s<m_bTempDCardCount;s++)
							{
								for(n=0;n<m_bTempGetCardCount;n++)
								{
									if(m_GameLogic.GetCardLogicValue(m_bTempSCardData[m_bTempSCardCount-j-1])==m_GameLogic.GetCardLogicValue(m_bTempDCardData[s]))
										m_bIsHaveCard=true;
								}
							}
							if(m_bTempGetCardCount==AnalyseResult.bThreeCount*3||!m_bIsHaveCard)
							{
								m_bTempGetCardData[AnalyseResult.bThreeCount*3+m]=m_bTempSCardData[m_bTempSCardCount-j-1];
								m_bTempGetCardCount=AnalyseResult.bThreeCount*3+m+1;
							}
							if(!m_bIsHaveCard)
							{
								n=m_bTempGetCardCount;
								j=m_bTempSCardCount;
							}
						}
					}
				}
			}
		}
		break;
	case CT_FOUR_LINE_TAKE_ONE:
	case CT_FOUR_LINE_TAKE_DOUBLE:
		{
			//分析扑克
			tagAnalyseResult AnalyseResult;
			m_GameLogic.AnalysebCardData(m_bTurnCardData,m_bTurnCardCount,AnalyseResult);
			if(m_WhichOnsKindCard==1)       //判断是不是具有唯一性
			{
				for(i=m_bTempFCardCount;i>0;i--)
				{
					if(i-AnalyseResult.bFourCount*4>=0&&m_GameLogic.CompareCard(&m_bTempFCardData[i-AnalyseResult.bFourCount*4],m_bTurnCardData,AnalyseResult.bFourCount*4,AnalyseResult.bFourCount*4))
					{
						if((m_bWhichKindSel++)>1)
							i=0;
					}
				}
			}
			for(i=m_bTempFCardCount;i>0;i--)
			{
				if(i-AnalyseResult.bFourCount*4>=0&&m_GameLogic.CompareCard(&m_bTempFCardData[i-AnalyseResult.bFourCount*4],m_bTurnCardData,AnalyseResult.bFourCount*4,AnalyseResult.bFourCount*4))
				{
					CopyMemory(m_bTempGetCardData,&m_bTempFCardData[i-AnalyseResult.bFourCount*4],AnalyseResult.bFourCount*4);
					m_bTempGetCardCount=AnalyseResult.bFourCount*4;
					i=0;
				}
			}
			if(m_bTempGetCardCount>0)
			{
				bool m_bIsHaveSame;
				for(int m=0;m<AnalyseResult.bDoubleCount;m++)
				{
					for(int j=0;j<m_bTempDCardCount/2;j++)
					{
						//判断是不是最合理的
						m_bIsHaveSame=false;
						for(int n=0;n<m_bTempGetCardCount;n++)
						{
							if(m_GameLogic.GetCardLogicValue(m_bTempDCardData[m_bTempDCardCount-j*2-1])==m_GameLogic.GetCardLogicValue(m_bTempGetCardData[n]))
							{
								m_bIsHaveSame=true;
							}
						}
						if(!m_bIsHaveSame)
						{
							bool m_bIsHaveCard=false;
							for(int s=0;s<m_bTempTCardCount;s++)
							{
								for(int n=0;n<m_bTempGetCardCount;n++)
								{
									if(m_GameLogic.GetCardLogicValue(m_bTempDCardData[m_bTempDCardCount-j*2-1])==m_GameLogic.GetCardLogicValue(m_bTempTCardData[s]))
										m_bIsHaveCard=true;
								}
							}
							if(m_bTempGetCardCount==AnalyseResult.bFourCount*4||!m_bIsHaveCard)
							{
								m_bTempGetCardData[AnalyseResult.bFourCount*4+m*2]=m_bTempDCardData[m_bTempDCardCount-j*2-1];
								m_bTempGetCardData[AnalyseResult.bFourCount*4+m*2+1]=m_bTempDCardData[m_bTempDCardCount-j*2-2];
								m_bTempGetCardCount=AnalyseResult.bFourCount*4+(m+1)*2;
							}
							if(!m_bIsHaveCard)
							{
								n=m_bTempGetCardCount;
								j=m_bTempDCardCount/2;
							}
						}
					}
				}
				for(int m=0;m<AnalyseResult.bSignedCount;m++)
				{
					for(int j=0;j<m_bTempSCardCount;j++)
					{
						//判断是不是最合理的
						m_bIsHaveSame=false;
						for(int n=0;n<m_bTempGetCardCount;n++)
						{
							if(m_GameLogic.GetCardLogicValue(m_bTempSCardData[m_bTempSCardCount-j-1])==m_GameLogic.GetCardLogicValue(m_bTempGetCardData[n]))
							{
								m_bIsHaveSame=true;
							}
						}
						if(!m_bIsHaveSame)
						{
							bool m_bIsHaveCard=false;
							for(int s=0;s<m_bTempDCardCount;s++)
							{
								for(int n=0;n<m_bTempGetCardCount;n++)
								{
									if(m_GameLogic.GetCardLogicValue(m_bTempSCardData[m_bTempSCardCount-j-1])==m_GameLogic.GetCardLogicValue(m_bTempDCardData[j]))
										m_bIsHaveCard=true;
								}
							}
							if(m_bTempGetCardCount==AnalyseResult.bFourCount*4||!m_bIsHaveCard)
							{
								m_bTempGetCardData[AnalyseResult.bFourCount*4+m]=m_bTempSCardData[m_bTempSCardCount-j-1];
								m_bTempGetCardCount=AnalyseResult.bFourCount*4+m+1;
							}
							if(!m_bIsHaveCard)
							{
								n=m_bTempGetCardCount;
								j=m_bTempSCardCount;
							}
						}
					}
				}
			}
		}
		break;
	}
	if(m_bTempGetCardCount==0)
	{
		m_bWhichKindSel=0;
		//判断炸弹的可能性
		if(m_bTempFCardCount>3)
		{
			for(i=m_bTempFCardCount-4;i>=0;i--)
			{
				if(m_GameLogic.CompareCard(&m_bTempFCardData[i],m_bTurnCardData,4,m_bTurnCardCount))
				{
					if((m_bWhichKindSel++)==0)
					{
						CopyMemory(m_bTempGetCardData,&m_bTempFCardData[i],4);
						m_bTempGetCardCount=4;
					}
				}
			}
		}
		if(m_bTempGetCardCount==0)
		{
			if(m_bHandCardCount>1)
			{
				if(m_GameLogic.GetCardLogicValue(m_bHandCardData[0])>15&&m_GameLogic.GetCardLogicValue(m_bHandCardData[1])>15)
				{
					CopyMemory(m_bTempGetCardData,m_bHandCardData,2);
					m_bTempGetCardCount=2;
					if(m_WhichOnsKindCard==1)
						m_bWhichKindSel=1;
				}
			}
		}
	}
	BYTE m_GetIndex=0;
	if(m_bTempGetCardCount==0)
	{
		if(m_WhichOnsKindCard!=1)
			OnPassCard(0,0);
	}
	else
	{
		for(int j=0;j<m_bTempGetCardCount;j++)
		{
			for(i=0;i<m_bHandCardCount;i++)
			{
				if(m_bHandCardData[i]==m_bTempGetCardData[j])
				{
					m_bTempGetCardIndex[m_GetIndex++]=i;
				}
			}
		}

	}
	if(m_GameLogic.CompareCard(m_bTempGetCardData,m_bTurnCardData,m_bTempGetCardCount,m_bTurnCardCount))
	{
		if(m_WhichOnsKindCard==1&&m_bWhichKindSel==1||m_WhichOnsKindCard!=1)
		{
			m_GameClientView.m_HandCardControl.SetShootCard(m_bTempGetCardIndex,m_GetIndex);
			m_GameClientView.m_btOutCard.EnableWindow(TRUE);
		}
	}
	else
	{
		if(m_WhichOnsKindCard!=1)
			OnPassCard(0,0);
	}
}

//调整视频窗口
void CGameClientDlg::ResetVideoWindowPostion(int wInt, int hInt)
{
	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sResetVideoWindowPostion%d.log",strTime,GetMeChairID()) ;

	strTemp.Format("%s", "ResetVideoWindowPostion()");
	WriteLog(strFile, strTemp);

	uiShowInt++;
	//设置自己的视频显示窗口
	int myX,myY,myW,myH;
	myW= MYSLEF_VIDEO_W;
	myH= MYSLEF_VIDEO_H;
	myX= wInt-myW-10;//
	myY= hInt-myH-10; 
	m_showSelfVideo.MoveWindow( myX, myY, myW, myH);
	if ( 1== uiShowInt)
	m_showSelfVideo.ShowWindow(true);
	//自己工具栏
	if ( 1== uiShowInt)
	m_MyselfOpKuang.ShowWindow(true);
	m_MyselfOpKuang.MoveWindow(myX, myY+myH+15, 80, 20);
	//动态计算X,Y
	int x,y,w,h;
	int xx,yy,ww,hh;

	int xOff = wInt;
	int yOff = 25;//m_GameClientView.yOffInt;



	strTemp.Format("xOff = %d", xOff);
	WriteLog(strFile, strTemp);

	CString showMsg;
	showMsg.Format("xOff=%d,yOff=%d", xOff, yOff);

	int screenInt = GetSystemMetrics(SM_CXSCREEN);
	ww= AV_TOOLS_ALL_BUTTON_W  ;
	hh= AV_TOOLS_BUTTON_H  ;

	int opW,opH;
	opW = AV_TOOLS_W  ;
	opH = AV_TOOLS_H + 8 ;


	w= VIDEO_W;
	h= VIDEO_H;


	int topX = 280+xOff;
	int topY = 34+ yOff;

	x = 8 ;//+ xOff;
	y = topY;

	int rightX= xOff - (VIDEO_PIC_W+OP_BACK_W) ;//577+xOff;

	if ( 800 == screenInt)
	{
		y = 70;
	}
	else if (1152 == screenInt)
	{
		y = 210;
	}


	int myDeskInt = GetMeChairID();//= GetMeUserInfo()->bDeskStation;

	if ( myDeskInt > GAME_PLAYER)
	{
		strTemp.Format("myDeskInt=%d",myDeskInt);
		WriteLog(strFile, strTemp);
		m_GameClientView.xOffInt = -1;
		m_GameClientView.yOffInt = -1;
		uiShowInt=0;
		return ;
	}
	else
	{
		strTemp.Format("myDeskInt=%d",myDeskInt);
		WriteLog(strFile, strTemp);
	}


	CStatic * myVideoList[GAME_PLAYER] = {&m_showRemoteVideo, &m_showRemoteVideo1, &m_showRemoteVideo2};
	CLabelEx * myToolsList[GAME_PLAYER]={&m_OpKuang, &m_OpKuang1, &m_OpKuang2};

	strTemp.Format("a1");
//	WriteLog(strFile, strTemp);

	for (int i=0; i < GAME_PLAYER; i++)
	{
		//自己不显示
		if ( i == myDeskInt)
		{
			m_GameClientView.m_uVideoInt[i] = 0;
			myVideoList[i]->ShowWindow(false);
			myToolsList[i]->ShowWindow(false);
		}
		else
		{
			m_GameClientView.m_uVideoInt[i] = 1;

			if ( 1== uiShowInt)
			{
			myVideoList[i]->ShowWindow(true);
			myToolsList[i]->ShowWindow(true);
			}

		}

	}//End for
	strTemp.Format("a2");
//	WriteLog(strFile, strTemp);

	//left top right
	CStatic * myVideo[GAME_PLAYER-1];//视频组件
	CLabelEx * myTools[GAME_PLAYER-1];//工具组件
	CPoint * myPt[GAME_PLAYER-1];//位置
	int listInt [GAME_PLAYER-1]={1, 2};//显示

	if ( myDeskInt > (GAME_PLAYER-1) || myDeskInt < 0)
	{
		strTemp.Format("retrun ");
		WriteLog(strFile, strTemp);
		return ;

	}//End if
	//顺时钟
	if (m_bDeasilOrder==true)
	{
		switch(myDeskInt)
		{
		case 0:
			{
				//
				listInt[0]=1;
				listInt[1]=2;

			}
			break;
		case 1:
			{
				//0,2,3
				listInt[0]=2;
				listInt[1]=0;


			}
			break;
		case 2:
			{
				//0,1,3

				listInt[0]=0;
				listInt[1]=1;


			}
			break;

		}
	}
	else
	{
		//逆时针
		//2,1,0,2,1,0
		switch(myDeskInt)
		{
		case 0:
			{

				listInt[0]=2;
				listInt[1]=1;

			}
			break;
		case 1:
			{
				//0,2,3
				listInt[0]=0;
				listInt[1]=2;


			}
			break;
		case 2:
			{
				//0,1,3

				listInt[0]=1;
				listInt[1]=0;


			}
			break;

		}	
	}

	strTemp.Format("a3");
//	WriteLog(strFile, strTemp);
	try
	{
		for (int i=0; i < GAME_PLAYER-1; i++)
		{
			int videoInt= listInt[i];
			strTemp.Format("videoInt=%d, i=%d", videoInt, i);
			WriteLog(strFile, strTemp);

			myVideo[i]=myVideoList[videoInt];
			myTools[i]=myToolsList[videoInt];

			if (myVideo[i] == NULL || myTools[i] == NULL)
				continue;
			myPt[i]=&m_GameClientView.m_PtVideo[videoInt];
			strTemp.Format("b1");
//			WriteLog(strFile, strTemp);

			if ( i==1 )
			{	
				x = rightX;
			}

			myVideo[i]->MoveWindow( x, y, w, h);

			strTemp.Format("视频窗口:videoInt=%d, x=%d, y=%d, w=%d, h=%d", videoInt,x, y, w, h);
			WriteLog(strFile, strTemp);
			myPt[i]->x =x-VIDEO_X_OFF;
			myPt[i]->y =y-VIDEO_Y_OFF;

			xx = x+opW;
			yy = y+opH;
			myTools[i]->MoveWindow(xx, yy, ww, hh);
			strTemp.Format("xx=%d, yy=%d, ww=%d, hh=%d", xx, yy, ww, hh);
			WriteLog(strFile, strTemp);
		}//End for
	}
	catch(...)
	{
		strTemp.Format("bad!!!");
	}
	strTemp.Format("a4");
	WriteLog(strFile, strTemp);

	//	m_GameClientView.UpdateViewFace(NULL);

	m_GameClientView.UpdateGameView(NULL);

	strTemp.Format("UpdateGameView");
	WriteLog(strFile, strTemp);
}

//RESET UI
LRESULT CGameClientDlg::OnResetUI(WPARAM wParam, LPARAM lParam)
{
	int wInt=(int)wParam;
	int hInt=(int)lParam;

	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sOnResetUI.log",strTime);

	strTemp.Format("into OnResetUI(%d, %d", wInt, hInt);
	WriteLog(strFile, strTemp);

	if ( wInt >0 && wInt < 1500)
	{
		ResetVideoWindowPostion( wInt,  hInt);
	}
	return 0;
}
//播放OutCard声音
//输入:牌类型
void CGameClientDlg::PlayOutCardSound(int sexInt, int cardTypeInt, int cardInt)
{
	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sCClientGameDlg.log",strTime);

	strTemp.Format("into PlayOutCardSound(cardTypeInt=%d, cardInt=%d", cardTypeInt, cardInt);
	WriteLog(strFile, strTemp);


	CString mp3FileStr;

	CString sexStr="boy";
	int baseDuiZi = 55;

	if ( sexInt != 1)
	{
		sexStr="girl";
		baseDuiZi = 58;
	}
	//	if ( pUserInfo
	CString headStr=".\\DDZ\\sound";
	CString folderStr="";

	switch(cardTypeInt)
	{

	case CT_SINGLE					:				//单牌
		{
			folderStr= "danzhang";
			mp3FileStr.Format("%s\\%s\\%s\\%d\\%s-%d.mp3", headStr, sexStr, folderStr, cardInt, sexStr,  cardInt);

			if ( mp3FileStr.GetLength() > 0 )
			{
				strTemp.Format("mp3FileStr %s", mp3FileStr);
				WriteLog(strFile, strTemp);


				g_myCFlyGameSound.PlayMP3Sound(mp3FileStr);	
			}
			return;
		}
		break;
	case CT_DOUBLE					:				//对牌
		{
			folderStr= "duizi";

			mp3FileStr.Format("%s\\%s\\%s\\%d\\%s (%d).mp3", headStr, sexStr, folderStr, cardInt, sexStr,  baseDuiZi +cardInt);

			if ( mp3FileStr.GetLength() > 0 )
			{
				strTemp.Format("mp3FileStr %s", mp3FileStr);
				WriteLog(strFile, strTemp);


				g_myCFlyGameSound.PlayMP3Sound(mp3FileStr);	
			}
			return ;
		}
		break;
	case CT_THREE					:				//三条
		{
			folderStr= "3zhang";
		}
		break;


	case CT_THREE_LINE_TAKE_ONE			:				//三带一
		{
			folderStr= "3vs1";
		}
		break;
	case CT_ONE_LINE			:				//单顺
		{
			folderStr= "danshun";
		}
		break;
	case CT_DOUBLE_LINE			:				//双顺
		{
			folderStr= "shuangshun";
		}
		break;
	case CT_THREE_LINE			:				//三顺
		{
			folderStr= "3shun";
		}
		break;
	case CT_THREE_LINE_TAKE_DOUBLE			:				//飞机带翅膀
		{
			folderStr= "3vs2";
		}
		break;
	case CT_FOUR_LINE_TAKE_DOUBLE				:				//四带二
		{
			folderStr= "4vs2";
		}
		break;
	case CT_BOMB_CARD						:				//炸弹
		{		
			folderStr= "bomb";
		}
		break;
	case CT_MISSILE_CARD						:				//火箭
		{
			folderStr= "rocket";
		}
		break;
	}

	if ( folderStr.GetLength() > 0 )
	{
		mp3FileStr.Format("%s\\%s\\%s\\%s-%s.mp3", headStr, sexStr, folderStr, sexStr,folderStr);
	}
	if ( mp3FileStr.GetLength() > 0 )
	{
		strTemp.Format("mp3FileStr %s", mp3FileStr);
		WriteLog(strFile, strTemp);


		g_myCFlyGameSound.PlayMP3Sound(mp3FileStr);	
		//PlayCardMovie( cardTypeInt);
	}	
}
//播放PASS声音
//输入
void CGameClientDlg::PlayPassSound(int sexInt)
{
	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sCClientGameDlg.log",strTime);

	strTemp.Format("into CClientGameDlg(");
	//	WriteLog(strFile, strTemp);


	CString mp3FileStr;

	CString sexStr="boy";
	int baseInt = 7;

	if ( sexInt != 1)
	{
		sexStr="girl";
		baseInt = 9;
	}
	//	if ( pUserInfo
	CString headStr=".\\DDZ\\sound";
	CString folderStr="";

	//声音随机数
	int soundNum = baseInt+rand()%4;
	mp3FileStr.Format("%s\\%s\\pass\\%s (%d).mp3", headStr,sexStr,sexStr,soundNum );		



	if ( mp3FileStr.GetLength() > 0 )
	{
		strTemp.Format("mp3FileStr %s", mp3FileStr);
		WriteLog(strFile, strTemp);


		g_myCFlyGameSound.PlayMP3Sound(mp3FileStr);	
	}	
}
//播放OutCard声音
//输入:牌类型
void CGameClientDlg::PlayCardMovie(int cardTypeInt)
{
	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sPlayCardMovie.log",strTime);

	strTemp.Format("into PlayCardMovie(cardTypeInt=%d", cardTypeInt);
	WriteLog(strFile, strTemp);


	switch(cardTypeInt)
	{

	case CT_THREE_LINE_TAKE_DOUBLE			:				//飞机带翅膀
		{
			m_GameClientView.myMoiveList[1].useStatus = 1;
			m_GameClientView.myMoiveList[1].typeInt = 1;
			m_GameClientView.myMoiveList[1].myPoint.x = GetSystemMetrics(SM_CXSCREEN);
			m_GameClientView.myMoiveList[1].myPoint.y = 350;
			m_GameClientView.myMoiveList[1].maxInt = 2;
			m_GameClientView.myMoiveList[1].currentInt = 0;

			m_GameClientView.myMoiveList[1].addX=-30;//位移
			m_GameClientView.myMoiveList[1].addY=0;//位移
			m_GameClientView.myMoiveList[1].picW=227;
			m_GameClientView.myMoiveList[1].picH=131;

			strTemp.Format("into PlayCardMovie(CT_THREE_LINE_TAKE_DOUBLE");
			WriteLog(strFile, strTemp);

			SetTimer( ID_SHOW_PLANE,30, NULL);//
			//	PlaySound(TEXT("SOUND_PLANE"),GetModuleHandle(CLIENT_DLL_NAME),SND_RESOURCE|SND_NOSTOP|SND_ASYNC|SND_NODEFAULT|SND_NOWAIT);
			//PlaySound( IDR_WAVE1,GetModuleHandle(CLIENT_DLL_NAME),SND_RESOURCE|SND_NOSTOP|SND_ASYNC|SND_NODEFAULT|SND_NOWAIT);
			PlayGameSound(AfxGetInstanceHandle(),TEXT("SOUND_PLANE"));
		}
		break;

	case CT_BOMB_CARD						:				//炸弹
		{		
			m_GameClientView.myMoiveList[0].useStatus = 1;
			m_GameClientView.myMoiveList[0].typeInt = 0;
			m_GameClientView.myMoiveList[0].myPoint.x = 250;
			m_GameClientView.myMoiveList[0].myPoint.y = 150;
			m_GameClientView.myMoiveList[0].maxInt = 7;
			m_GameClientView.myMoiveList[0].currentInt = 0;

			m_GameClientView.myMoiveList[1].addX=0;//位移
			m_GameClientView.myMoiveList[1].addY=0;//位移
			m_GameClientView.myMoiveList[1].picW=161;
			m_GameClientView.myMoiveList[1].picH=151;

			strTemp.Format("into PlayCardMovie(CT_BOMB_CARD");
			WriteLog(strFile, strTemp);

			SetTimer( ID_SHOW_BOMB,100, NULL);//30 S
			//		PlaySound(TEXT("SOUND_BOMB"),GetModuleHandle(CLIENT_DLL_NAME),SND_RESOURCE|SND_NOSTOP|SND_ASYNC|SND_NODEFAULT|SND_NOWAIT);
			PlayGameSound(AfxGetInstanceHandle(),TEXT("SOUND_BOMB"));
		}
		break;
	case CT_MISSILE_CARD						:				//火箭
		{
			m_GameClientView.myMoiveList[2].useStatus = 1;
			m_GameClientView.myMoiveList[2].typeInt = 2;
			m_GameClientView.myMoiveList[2].myPoint.x = 250;
			m_GameClientView.myMoiveList[2].myPoint.y = GetSystemMetrics(SM_CYSCREEN);
			m_GameClientView.myMoiveList[2].maxInt=4;
			m_GameClientView.myMoiveList[2].currentInt = 0;

			m_GameClientView.myMoiveList[2].addX=0;//位移
			m_GameClientView.myMoiveList[2].addY=-30;//位移
			m_GameClientView.myMoiveList[2].picW=107;
			m_GameClientView.myMoiveList[2].picH=290;

			strTemp.Format("into PlayCardMovie(CT_MISSILE_CARD	");
			WriteLog(strFile, strTemp);

			SetTimer( ID_SHOW_ROCKET,50, NULL);//30 S
			//			PlaySound(TEXT("SOUND_ROCKET"),GetModuleHandle(CLIENT_DLL_NAME),SND_RESOURCE|SND_NOSTOP|SND_ASYNC|SND_NODEFAULT|SND_NOWAIT);
			PlayGameSound(AfxGetInstanceHandle(),TEXT("SOUND_ROCKET"));

		}
		break;
	}

}
//输入：1，2，3，NO
void CGameClientDlg::PlayJiaoFenSound(int sexInt,int fenInt)
{
	CString strFile,strTemp;
	CTime tmCur = CTime::GetCurrentTime();
	CString strTime = tmCur.Format("%m%d");
	strFile.Format("log\\%sHandleGameMessage.log",strTime);

	strTemp.Format("into PlayJiaoFenSound(%d,", fenInt);
	WriteLog(strFile, strTemp);

	CString mp3FileStr;

	CString sexStr="boy";

	if ( sexInt != 1)
	{
		sexStr="girl";
	}
	//	if ( pUserInfo
	CString headStr=".\\DDZ\\sound";
	CString folderStr="";


	switch(fenInt)
	{
	case 1:	
	case 2:
	case 3:
		{
			mp3FileStr.Format("%s\\%s\\%dfen\\%s-%dfen.mp3", headStr, sexStr, fenInt, sexStr,fenInt);		
		}
		break;
	}

	if ( mp3FileStr.GetLength() > 0 )
	{
		strTemp.Format("mp3FileStr %s", mp3FileStr);
		WriteLog(strFile, strTemp);


		g_myCFlyGameSound.PlayMP3Sound(mp3FileStr);	
	}
}
//////////////////////////////////////////////////////////////////////////

