#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "s2clientprotocol/sc2api.pb.h"
#include "common/planning/EIntentDomain.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_map_info.h"
#include "terran_unit_container.h"

namespace sc2
{

struct FFrameContext
{
    const ObservationInterface* Observation{nullptr};
    QueryInterface* Query{nullptr};
    const SC2APIProtocol::Observation* RawObservation{nullptr};
    const GameInfo* GameInfo{nullptr};
    Point2D CameraWorld;
    uint64_t CurrentStep{0};
    uint64_t GameLoop{0};

    static FFrameContext Create(const ObservationInterface* ObservationPtr, QueryInterface* QueryPtr, uint64_t CurrentStepValue)
    {
        FFrameContext FrameContextValue;
        FrameContextValue.Observation = ObservationPtr;
        FrameContextValue.Query = QueryPtr;
        FrameContextValue.CurrentStep = CurrentStepValue;
        if (ObservationPtr)
        {
            FrameContextValue.RawObservation = ObservationPtr->GetRawObservation();
            FrameContextValue.GameInfo = &ObservationPtr->GetGameInfo();
            FrameContextValue.CameraWorld = ObservationPtr->GetCameraPos();
            FrameContextValue.GameLoop = ObservationPtr->GetGameLoop();
        }
        return FrameContextValue;
    }

    bool IsValid() const
    {
        return Observation != nullptr && GameInfo != nullptr;
    }
};

inline Point2D ClampToPlayable(const GameInfo& GameInfoData, const Point2D& PointValue)
{
    return Point2D(std::clamp(PointValue.x, GameInfoData.playable_min.x, GameInfoData.playable_max.x),
                   std::clamp(PointValue.y, GameInfoData.playable_min.y, GameInfoData.playable_max.y));
}

inline bool IsPointFinite(const Point2D& PointValue)
{
    return std::isfinite(PointValue.x) && std::isfinite(PointValue.y);
}

inline bool IsGroundPathingResultValid(const Unit& ActorUnitValue, const Point2D& TargetPointValue,
                                       const float PathingDistanceValue)
{
    if (!std::isfinite(PathingDistanceValue) || PathingDistanceValue < 0.0f)
    {
        return false;
    }

    constexpr float SamePointToleranceSquared = 0.25f;
    const float DirectDistanceSquared = DistanceSquared2D(Point2D(ActorUnitValue.pos), TargetPointValue);
    if (PathingDistanceValue <= 0.0f && DirectDistanceSquared > SamePointToleranceSquared)
    {
        return false;
    }

    return true;
}

struct FSpatialChannel8BPP
{
    int Width{0};
    int Height{0};
    int BitsPerPixel{0};
    std::string Data;

    void Reset()
    {
        Width = 0;
        Height = 0;
        BitsPerPixel = 0;
        Data.clear();
    }

    bool IsValid() const
    {
        return BitsPerPixel == 8 && Width > 0 && Height > 0 && Data.size() == static_cast<size_t>(Width * Height);
    }

    bool InBounds(const Point2DI& PointValue) const
    {
        return PointValue.x >= 0 && PointValue.x < Width && PointValue.y >= 0 && PointValue.y < Height;
    }

    uint8_t Read(const Point2DI& PointValue) const
    {
        if (!InBounds(PointValue))
        {
            return 0;
        }

        return static_cast<uint8_t>(static_cast<unsigned char>(Data[PointValue.x + PointValue.y * Width]));
    }

    bool Load(const SC2APIProtocol::ImageData& ImageDataValue)
    {
        Reset();

        if (!ImageDataValue.has_size())
        {
            return false;
        }

        Width = ImageDataValue.size().x();
        Height = ImageDataValue.size().y();
        BitsPerPixel = ImageDataValue.bits_per_pixel();
        Data = ImageDataValue.data();

        if (!IsValid())
        {
            Reset();
            return false;
        }

        return true;
    }
};

struct FSpatialOccupancySummary
{
    uint32_t SelfCount{0};
    uint32_t EnemyCount{0};
    uint32_t NeutralCount{0};

    bool HasSelf{false};
    bool HasEnemy{false};
    bool HasNeutral{false};

    Point2D SelfCentroid;
    Point2D EnemyCentroid;
    Point2D NeutralCentroid;

    bool HasSelfBounds{false};
    bool HasEnemyBounds{false};
    bool HasNeutralBounds{false};

    Rect2DI SelfBounds;
    Rect2DI EnemyBounds;
    Rect2DI NeutralBounds;

    float SelfSumX{0.0f};
    float SelfSumY{0.0f};
    float EnemySumX{0.0f};
    float EnemySumY{0.0f};
    float NeutralSumX{0.0f};
    float NeutralSumY{0.0f};

    void Reset()
    {
        SelfCount = 0;
        EnemyCount = 0;
        NeutralCount = 0;
        HasSelf = false;
        HasEnemy = false;
        HasNeutral = false;
        SelfCentroid = Point2D();
        EnemyCentroid = Point2D();
        NeutralCentroid = Point2D();
        HasSelfBounds = false;
        HasEnemyBounds = false;
        HasNeutralBounds = false;
        SelfBounds = Rect2DI();
        EnemyBounds = Rect2DI();
        NeutralBounds = Rect2DI();
        SelfSumX = 0.0f;
        SelfSumY = 0.0f;
        EnemySumX = 0.0f;
        EnemySumY = 0.0f;
        NeutralSumX = 0.0f;
        NeutralSumY = 0.0f;
    }

    static void UpdateBounds(Rect2DI& Bounds, bool& HasBounds, int X, int Y)
    {
        if (!HasBounds)
        {
            Bounds = Rect2DI(Point2DI(X, Y), Point2DI(X, Y));
            HasBounds = true;
            return;
        }

        Bounds.from.x = std::min(Bounds.from.x, X);
        Bounds.from.y = std::min(Bounds.from.y, Y);
        Bounds.to.x = std::max(Bounds.to.x, X);
        Bounds.to.y = std::max(Bounds.to.y, Y);
    }

    void Accumulate(uint8_t Value, int X, int Y)
    {
        switch (Value)
        {
            case 1:
            case 2:
                ++SelfCount;
                SelfSumX += static_cast<float>(X);
                SelfSumY += static_cast<float>(Y);
                UpdateBounds(SelfBounds, HasSelfBounds, X, Y);
                break;
            case 3:
                ++NeutralCount;
                NeutralSumX += static_cast<float>(X);
                NeutralSumY += static_cast<float>(Y);
                UpdateBounds(NeutralBounds, HasNeutralBounds, X, Y);
                break;
            case 4:
                ++EnemyCount;
                EnemySumX += static_cast<float>(X);
                EnemySumY += static_cast<float>(Y);
                UpdateBounds(EnemyBounds, HasEnemyBounds, X, Y);
                break;
            default:
                break;
        }
    }

    void Finalize()
    {
        HasSelf = SelfCount > 0;
        HasEnemy = EnemyCount > 0;
        HasNeutral = NeutralCount > 0;

        if (HasSelf)
        {
            SelfCentroid = Point2D(SelfSumX / static_cast<float>(SelfCount), SelfSumY / static_cast<float>(SelfCount));
        }
        if (HasEnemy)
        {
            EnemyCentroid = Point2D(EnemySumX / static_cast<float>(EnemyCount),
                                    EnemySumY / static_cast<float>(EnemyCount));
        }
        if (HasNeutral)
        {
            NeutralCentroid = Point2D(NeutralSumX / static_cast<float>(NeutralCount),
                                      NeutralSumY / static_cast<float>(NeutralCount));
        }
    }
};

struct FAgentSpatialChannels
{
    bool Valid{false};

    FSpatialChannel8BPP MapPlayerRelative;
    FSpatialChannel8BPP MinimapPlayerRelative;
    FSpatialChannel8BPP MinimapHeightMap;

    SpatialSetup FeatureLayerSetup{};
    Point2D CameraWorld;
    Point2D PlayableMin;
    Point2D PlayableMax;
    int MapWorldWidth{0};
    int MapWorldHeight{0};

    void Reset()
    {
        Valid = false;
        MapPlayerRelative.Reset();
        MinimapPlayerRelative.Reset();
        MinimapHeightMap.Reset();
        FeatureLayerSetup = SpatialSetup();
        CameraWorld = Point2D();
        PlayableMin = Point2D();
        PlayableMax = Point2D();
        MapWorldWidth = 0;
        MapWorldHeight = 0;
    }

    void Update(const FFrameContext& Frame)
    {
        Reset();

        if (!Frame.IsValid() || Frame.RawObservation == nullptr || !Frame.RawObservation->has_feature_layer_data())
        {
            return;
        }

        const SC2APIProtocol::ObservationFeatureLayer& FeatureLayerData = Frame.RawObservation->feature_layer_data();
        if (!FeatureLayerData.has_renders() || !FeatureLayerData.has_minimap_renders())
        {
            return;
        }

        const SC2APIProtocol::FeatureLayers& MapLayers = FeatureLayerData.renders();
        const SC2APIProtocol::FeatureLayersMinimap& MinimapLayers = FeatureLayerData.minimap_renders();
        if (!MapLayers.has_player_relative() || !MinimapLayers.has_player_relative() || !MinimapLayers.has_height_map())
        {
            return;
        }

        if (!MapPlayerRelative.Load(MapLayers.player_relative()) || !MinimapPlayerRelative.Load(MinimapLayers.player_relative()) ||
            !MinimapHeightMap.Load(MinimapLayers.height_map()))
        {
            Reset();
            return;
        }

        FeatureLayerSetup = Frame.GameInfo->options.feature_layer;
        CameraWorld = Frame.CameraWorld;
        PlayableMin = Frame.GameInfo->playable_min;
        PlayableMax = Frame.GameInfo->playable_max;
        MapWorldWidth = Frame.GameInfo->width;
        MapWorldHeight = Frame.GameInfo->height;
        Valid = true;
    }

    Point2DI ConvertWorldToMinimap(const Point2D& WorldPosition) const
    {
        if (!Valid || FeatureLayerSetup.minimap_resolution_x <= 0 || FeatureLayerSetup.minimap_resolution_y <= 0 ||
            MapWorldWidth <= 0 || MapWorldHeight <= 0)
        {
            return Point2DI();
        }

        const float PixelSize = std::max(static_cast<float>(MapWorldWidth) /
                                              static_cast<float>(FeatureLayerSetup.minimap_resolution_x),
                                          static_cast<float>(MapWorldHeight) /
                                              static_cast<float>(FeatureLayerSetup.minimap_resolution_y));
        const float ImageRelativeX = WorldPosition.x;
        const float ImageRelativeY = static_cast<float>(MapWorldHeight) - WorldPosition.y;

        return Point2DI(static_cast<int>(ImageRelativeX / PixelSize),
                        static_cast<int>(ImageRelativeY / PixelSize));
    }

    Point2DI ConvertWorldToCamera(const Point2D& WorldPosition) const
    {
        if (!Valid || FeatureLayerSetup.camera_width <= 0.0f || FeatureLayerSetup.map_resolution_x <= 0 ||
            FeatureLayerSetup.map_resolution_y <= 0)
        {
            return Point2DI();
        }

        const float PixelSize = FeatureLayerSetup.camera_width /
                                 static_cast<float>(std::min(FeatureLayerSetup.map_resolution_x,
                                                             FeatureLayerSetup.map_resolution_y));
        const float ImageWidthWorld = PixelSize * static_cast<float>(FeatureLayerSetup.map_resolution_x);
        const float ImageHeightWorld = PixelSize * static_cast<float>(FeatureLayerSetup.map_resolution_y);

        const float ImageOriginX = CameraWorld.x - ImageWidthWorld / 2.0f;
        const float ImageOriginY = CameraWorld.y + ImageHeightWorld / 2.0f;
        const float ImageRelativeX = WorldPosition.x - ImageOriginX;
        const float ImageRelativeY = ImageOriginY - WorldPosition.y;

        return Point2DI(static_cast<int>(ImageRelativeX / PixelSize),
                        static_cast<int>(ImageRelativeY / PixelSize));
    }
};

struct FAgentSpatialMetrics
{
    bool Valid{false};
    FSpatialOccupancySummary Map;
    FSpatialOccupancySummary Minimap;

    void Reset()
    {
        Valid = false;
        Map.Reset();
        Minimap.Reset();
    }

    static void AccumulateChannel(const FSpatialChannel8BPP& Channel, FSpatialOccupancySummary& Summary)
    {
        Summary.Reset();
        if (!Channel.IsValid())
        {
            return;
        }

        for (int Y = 0; Y < Channel.Height; ++Y)
        {
            for (int X = 0; X < Channel.Width; ++X)
            {
                Summary.Accumulate(Channel.Read(Point2DI(X, Y)), X, Y);
            }
        }

        Summary.Finalize();
    }

    void Update(const FAgentSpatialChannels& Channels)
    {
        Reset();
        if (!Channels.Valid)
        {
            return;
        }

        AccumulateChannel(Channels.MapPlayerRelative, Map);
        AccumulateChannel(Channels.MinimapPlayerRelative, Minimap);
        Valid = true;
    }
};

enum class EIntentTargetKind : uint8_t
{
    None,
    Point,
    Unit,
};

struct FUnitIntent
{
    Tag ActorTag{NullTag};
    AbilityID Ability{ABILITY_ID::INVALID};
    EIntentTargetKind TargetKind{EIntentTargetKind::None};
    Point2D TargetPoint;
    Tag TargetUnitTag{NullTag};
    int Priority{0};
    EIntentDomain Domain{EIntentDomain::Recovery};
    bool Queued{false};
    bool RequiresPlacementValidation{false};
    bool RequiresPathingValidation{false};

    static FUnitIntent CreateNoTarget(Tag ActorTagValue, AbilityID AbilityValue, int PriorityValue, EIntentDomain DomainValue,
                                      bool QueuedValue = false)
    {
        FUnitIntent IntentValue;
        IntentValue.ActorTag = ActorTagValue;
        IntentValue.Ability = AbilityValue;
        IntentValue.Priority = PriorityValue;
        IntentValue.Domain = DomainValue;
        IntentValue.Queued = QueuedValue;
        return IntentValue;
    }

    static FUnitIntent CreatePointTarget(Tag ActorTagValue, AbilityID AbilityValue, const Point2D& PointValue, int PriorityValue,
                                         EIntentDomain DomainValue, bool RequiresPathingValidationValue = false,
                                         bool RequiresPlacementValidationValue = false, bool QueuedValue = false)
    {
        FUnitIntent IntentValue = CreateNoTarget(ActorTagValue, AbilityValue, PriorityValue, DomainValue, QueuedValue);
        IntentValue.TargetKind = EIntentTargetKind::Point;
        IntentValue.TargetPoint = PointValue;
        IntentValue.RequiresPathingValidation = RequiresPathingValidationValue;
        IntentValue.RequiresPlacementValidation = RequiresPlacementValidationValue;
        return IntentValue;
    }

    static FUnitIntent CreateUnitTarget(Tag ActorTagValue, AbilityID AbilityValue, Tag TargetTagValue, int PriorityValue,
                                        EIntentDomain DomainValue, bool QueuedValue = false)
    {
        FUnitIntent IntentValue = CreateNoTarget(ActorTagValue, AbilityValue, PriorityValue, DomainValue, QueuedValue);
        IntentValue.TargetKind = EIntentTargetKind::Unit;
        IntentValue.TargetUnitTag = TargetTagValue;
        return IntentValue;
    }

    bool Matches(const FUnitIntent& Other) const
    {
        return ActorTag == Other.ActorTag && Ability == Other.Ability && TargetKind == Other.TargetKind &&
               TargetUnitTag == Other.TargetUnitTag && TargetPoint == Other.TargetPoint &&
               Priority == Other.Priority && Domain == Other.Domain && Queued == Other.Queued &&
               RequiresPlacementValidation == Other.RequiresPlacementValidation &&
               RequiresPathingValidation == Other.RequiresPathingValidation;
    }
};

struct FIntentBuffer
{
    std::vector<FUnitIntent> Intents;

    void Reset()
    {
        Intents.clear();
    }

    void Add(const FUnitIntent& IntentValue)
    {
        Intents.push_back(IntentValue);
    }

    bool HasIntentForActor(Tag ActorTagValue) const
    {
        return std::any_of(Intents.begin(), Intents.end(),
                           [ActorTagValue](const FUnitIntent& IntentValue)
                           {
                               return IntentValue.ActorTag == ActorTagValue;
                           });
    }

    bool HasIntentForActorInDomain(Tag ActorTagValue, EIntentDomain DomainValue) const
    {
        return std::any_of(Intents.begin(), Intents.end(),
                           [ActorTagValue, DomainValue](const FUnitIntent& IntentValue)
                           {
                               return IntentValue.ActorTag == ActorTagValue && IntentValue.Domain == DomainValue;
                           });
    }
};

struct FIntentArbiter
{
    bool ShouldReplaceWinner(const FUnitIntent& Challenger, const FUnitIntent& Incumbent) const
    {
        if (Challenger.Priority != Incumbent.Priority)
        {
            return Challenger.Priority > Incumbent.Priority;
        }

        return GetIntentDomainOrder(Challenger.Domain) < GetIntentDomainOrder(Incumbent.Domain);
    }

    const Unit* FindActor(const FTerranUnitContainer& UnitContainerValue, Tag ActorTagValue) const
    {
        return UnitContainerValue.GetUnitByTag(ActorTagValue);
    }

    const Unit* FindTarget(const FFrameContext& Frame, Tag TargetTagValue) const
    {
        if (!Frame.Observation)
        {
            return nullptr;
        }

        return Frame.Observation->GetUnit(TargetTagValue);
    }

    bool ValidateAndNormalize(FUnitIntent& IntentValue, const FFrameContext& Frame, const FTerranUnitContainer& UnitContainerValue,
                              std::unordered_set<Tag>& ReservedBuildActors) const
    {
        const Unit* ActorUnit = FindActor(UnitContainerValue, IntentValue.ActorTag);
        if (!ActorUnit)
        {
            return false;
        }

        if (IntentValue.Domain == EIntentDomain::StructureBuild && !ReservedBuildActors.insert(IntentValue.ActorTag).second)
        {
            return false;
        }

        if (IntentValue.TargetKind == EIntentTargetKind::Unit)
        {
            return FindTarget(Frame, IntentValue.TargetUnitTag) != nullptr;
        }

        if (IntentValue.TargetKind == EIntentTargetKind::Point)
        {
            if (!Frame.GameInfo)
            {
                return false;
            }

            if (!IsPointFinite(IntentValue.TargetPoint))
            {
                return false;
            }

            IntentValue.TargetPoint = ClampToPlayable(*Frame.GameInfo, IntentValue.TargetPoint);

            if (IntentValue.RequiresPlacementValidation)
            {
                if (!Frame.Query || !Frame.Query->Placement(IntentValue.Ability, IntentValue.TargetPoint, ActorUnit))
                {
                    return false;
                }
            }

            if (IntentValue.RequiresPathingValidation && !ActorUnit->is_flying)
            {
                if (!Frame.Query)
                {
                    return false;
                }

                const float PathingDistanceValue = Frame.Query->PathingDistance(ActorUnit, IntentValue.TargetPoint);
                if (!IsGroundPathingResultValid(*ActorUnit, IntentValue.TargetPoint, PathingDistanceValue))
                {
                    return false;
                }
            }
        }

        return true;
    }

    std::vector<FUnitIntent> Resolve(const FFrameContext& Frame, const FTerranUnitContainer& UnitContainerValue,
                                     const FIntentBuffer& BufferValue) const
    {
        std::unordered_map<Tag, FUnitIntent> WinningIntents;
        for (const FUnitIntent& IntentValue : BufferValue.Intents)
        {
            if (IntentValue.ActorTag == NullTag)
            {
                continue;
            }

            std::unordered_map<Tag, FUnitIntent>::iterator FoundWinner = WinningIntents.find(IntentValue.ActorTag);
            if (FoundWinner == WinningIntents.end() || ShouldReplaceWinner(IntentValue, FoundWinner->second))
            {
                WinningIntents[IntentValue.ActorTag] = IntentValue;
            }
        }

        std::vector<FUnitIntent> ResolvedIntents;
        ResolvedIntents.reserve(WinningIntents.size());

        std::unordered_set<Tag> AddedActors;
        std::unordered_set<Tag> ReservedBuildActors;
        for (const FUnitIntent& OriginalIntent : BufferValue.Intents)
        {
            std::unordered_map<Tag, FUnitIntent>::iterator FoundWinner = WinningIntents.find(OriginalIntent.ActorTag);
            if (FoundWinner == WinningIntents.end())
            {
                continue;
            }
            if (AddedActors.find(OriginalIntent.ActorTag) != AddedActors.end())
            {
                continue;
            }
            if (!OriginalIntent.Matches(FoundWinner->second))
            {
                continue;
            }

            FUnitIntent NormalizedIntent = FoundWinner->second;
            if (!ValidateAndNormalize(NormalizedIntent, Frame, UnitContainerValue, ReservedBuildActors))
            {
                continue;
            }

            ResolvedIntents.push_back(NormalizedIntent);
            AddedActors.insert(NormalizedIntent.ActorTag);
        }

        return ResolvedIntents;
    }
};

}  // namespace sc2


