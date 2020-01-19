// Fill out your copyright notice in the Description page of Project Settings.


#include "clientGameModeBase.h"
#include "NetworkController.h"
#include "GameEventDispatcher.h"
#include "CompetitorRole.h"
#include "DataAdapter.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ConstructorHelpers.h"
AclientGameModeBase* AclientGameModeBase::smCurrentMode=nullptr;
AclientGameModeBase::AclientGameModeBase()
{
    UE_LOG(LogTemp, Display, TEXT("AclientGameModeBase::AclientGameModeBase") );
    smCurrentMode = this;
    PrimaryActorTick.bCanEverTick = true;
    //TODO 以下代码也可行
    //static ConstructorHelpers::FObjectFinder<UBlueprint> PutNameHere(TEXT("Blueprint'/Game/BluePrints/Competitor.Competitor'"));  
    //if (PutNameHere.Object)   
    //{  
    //    CompetitorClass = (UClass*)PutNameHere.Object->GeneratedClass;  
    //}  
}
AclientGameModeBase::~AclientGameModeBase()
{
    UE_LOG(LogTemp, Display, TEXT("AclientGameModeBase::~AclientGameModeBase") );
    NetworkController::GetInstance().Reset();
    mPlayerMap.Reset();
    if (mNewPlayerHandle.IsValid())
    {
        GameEventDispatcher::GetInstance().GetOnNewPlayer().Remove(mNewPlayerHandle);
        mNewPlayerHandle.Reset();
    }
    if (mMainPlayerSyncHandle.IsValid())
    {
        GameEventDispatcher::GetInstance().GetOnMainPlayerSync().Remove(mMainPlayerSyncHandle);
        mMainPlayerSyncHandle.Reset();
    }
}
ACompetitorRole* AclientGameModeBase::CreateACompetitorToLevel_Implementation(int _pid,
    const FString& _name,
    FVector _groundLocation,
    FRotator _rotation)
{
    auto newRole = this->GetWorld()->SpawnActor<ACompetitorRole>(
        CompetitorClass->GetDefaultObject()->GetClass(),
        _groundLocation,
        _rotation
        );
    if (newRole == nullptr)
    {
		UE_LOG(LogTemp, Error, TEXT("AclientGameModeBase::CreateAcompetitorToLevel_Implementation fail"));
        assert(false);
    }
    newRole->SetPid(_pid);
    newRole->SetPlayerName(_name);
    newRole->SetPlayerGroundLocation(_groundLocation);
    return newRole;
}
void AclientGameModeBase::Init()
{
    UE_LOG(LogTemp, Display, TEXT("AclientGameModeBase::Init") );
    NetworkController::GetInstance().Init(TEXT("127.0.0.1"),8899);
    UE_LOG(LogTemp, Display, TEXT("hahahahaha") );
    GameEventDispatcher::GetInstance().Init();
    this->mNewPlayerHandle = GameEventDispatcher::GetInstance().GetOnNewPlayer().AddUObject(this, &AclientGameModeBase::OnNewPlayer);
    this->mMainPlayerSyncHandle = GameEventDispatcher::GetInstance().GetOnMainPlayerSync().AddUObject(this, &AclientGameModeBase::OnSyncMainPlayerId);
    this->mMainPlayerLogoffHandle = GameEventDispatcher::GetInstance().GetOnPlayerLogoff().AddUObject(this, &AclientGameModeBase::OnPlayerLogoff);
}
void AclientGameModeBase::BeginPlay()
{
    UE_LOG(LogTemp, Display, TEXT("AclientGameModeBase::BeginPlay") );
    Super::BeginPlay();
}
void AclientGameModeBase::Tick(float deltaTime)
{
    Super::Tick(deltaTime);
    NetworkController::GetInstance().ProcessNetworkMessage();
}

void AclientGameModeBase::OnNewPlayer(int _pid, std::string _name,pb::Position _pos)
{
    UE_LOG(LogTemp, Display, TEXT("AclientGameModeBase::OnNewPlayer") );
	if (mPlayerMap.Contains(_pid))
	{
		UE_LOG(LogTemp, Error, TEXT("AclientGameModeBase::OnNewPlayer duplicate player"));
        return;
	}
    FVector location = DataAdapter::PostionSC(_pos);
    FRotator rot = FRotator::MakeFromEuler(FVector(0, 0, _pos.v()));
    auto role = this->CreateACompetitorToLevel(_pid, UTF8_TO_TCHAR(_name.c_str()),location,rot);
    if (role != nullptr)
    {
		UE_LOG(LogTemp, Display, TEXT("AclientGameModeBase::OnNewPlayer pid is : %d"), role->GetPid());
        this->RegisterPlayer(_pid, role);
    }
    else
    {
		UE_LOG(LogTemp, Error, TEXT("AclientGameModeBase::OnNewPlayer failed"));
        assert(false);
    }
}
void AclientGameModeBase::OnSyncMainPlayerId(APlayerRole *mainPlayer , int _pid)
{
    if (mMainPlayer != mainPlayer)
    {
        if (mMainPlayer != nullptr)
        {
            this->UnregisterPlayer(mMainPlayer->GetPid());
        }
        this->mMainPlayer = mainPlayer;
        this->RegisterPlayer(_pid, mainPlayer);
    }
}
void AclientGameModeBase::OnPlayerLogoff(int _pid)
{
    this->UnregisterPlayer(_pid);
}
void AclientGameModeBase::RegisterPlayer(int _pid, APlayerBase* _player)
{
    if (mPlayerMap.Contains(_pid))
    {
		UE_LOG(LogTemp, Error, TEXT("AclientGameModeBase::RegisterPlayer duplicate player"));
    }
    else
    {
		mPlayerMap.Add(_pid, _player);
    }
}
void AclientGameModeBase::UnregisterPlayer(int _pid)
{
    mPlayerMap.Remove(_pid);
}
