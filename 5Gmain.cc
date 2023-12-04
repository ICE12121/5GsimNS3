// Include statements
#include <random>
#include "ns3/antenna-module.h"
#include "ns3/random-variable-stream.h"
// #include "lte-ue-phy.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/eps-bearer-tag.h"
#include "ns3/grid-scenario-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"

using namespace ns3;


/**
 * \brief Simple RAN
 *
 * This example describes how to setup a simulation using the 3GPP channel model
 * from TR 38.900. This example consists of a simple topology of 1 UE and 1 gNb,
 * and only NR RAN part is simulated. One Bandwidth part and one CC are defined.
 * A packet is created and directly sent to gNb device by SendPacket function.
 * Then several functions are connected to PDCP and RLC traces and the delay is
 * printed.
 */

NS_LOG_COMPONENT_DEFINE ("5gMmWaveSimulation");



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// a single packet and directly calls the function send ////////////////////////////////////////
////////////////////////////////////////of a device to send the packet to the destination address. //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static bool g_rxPdcpCallbackCalled = false;
static bool g_rxRxRlcPDUCallbackCalled = false;

/**
 * Function creates a single packet and directly calls the function send
 * of a device to send the packet to the destination address.
 * @param device Device that will send the packet to the destination address.
 * @param addr Destination address for a packet.
 * @param packetSize The packet size.
 */
static void
SendPacket(Ptr<NetDevice> device, Address& addr, uint32_t packetSize)
{
    Ptr<Packet> pkt = Create<Packet>(packetSize);
    Ipv4Header ipv4Header;
    ipv4Header.SetProtocol(UdpL4Protocol::PROT_NUMBER);
    pkt->AddHeader(ipv4Header);
    EpsBearerTag tag(1, 1);
    pkt->AddPacketTag(tag);
    device->Send(pkt, addr, Ipv4L3Protocol::PROT_NUMBER);
}

/**
 * Function that prints out PDCP delay. This function is designed as a callback
 * for PDCP trace source.
 * @param path The path that matches the trace source
 * @param rnti RNTI of UE
 * @param lcid logical channel id
 * @param bytes PDCP PDU size in bytes
 * @param pdcpDelay PDCP delay
 */
void
RxPdcpPDU(std::string path, uint16_t rnti, uint8_t lcid, uint32_t bytes, uint64_t pdcpDelay)
{
    std::cout << "\n Packet PDCP delay:" << pdcpDelay << "\n";
    g_rxPdcpCallbackCalled = true;
}

/**
 * Function that prints out RLC statistics, such as RNTI, lcId, RLC PDU size,
 * delay. This function is designed as a callback
 * for RLC trace source.
 * @param path The path that matches the trace source
 * @param rnti RNTI of UE
 * @param lcid logical channel id
 * @param bytes RLC PDU size in bytes
 * @param rlcDelay RLC PDU delay
 */
void
RxRlcPDU(std::string path, uint16_t rnti, uint8_t lcid, uint32_t bytes, uint64_t rlcDelay)
{
    std::cout << "\n\n Data received at RLC layer at:" << Simulator::Now() << std::endl;
    std::cout << "\n rnti:" << rnti << std::endl;
    std::cout << "\n lcid:" << (unsigned)lcid << std::endl;
    std::cout << "\n bytes :" << bytes << std::endl;
    std::cout << "\n delay :" << rlcDelay << std::endl;
    g_rxRxRlcPDUCallbackCalled = true;
}

/**
 * Function that connects PDCP and RLC traces to the corresponding trace sources.
 */
void
ConnectPdcpRlcTraces()
{
    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/DataRadioBearerMap/1/LtePdcp/RxPDU",
                    MakeCallback(&RxPdcpPDU));

    Config::Connect("/NodeList/*/DeviceList/*/LteUeRrc/DataRadioBearerMap/1/LteRlc/RxPDU",
                    MakeCallback(&RxRlcPDU));
}


/**
 * Function that connects UL PDCP and RLC traces to the corresponding trace sources.
 */
void
ConnectUlPdcpRlcTraces()
{
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/DataRadioBearerMap/*/LtePdcp/RxPDU",
                    MakeCallback(&RxPdcpPDU));

    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/UeMap/*/DataRadioBearerMap/*/LteRlc/RxPDU",
                    MakeCallback(&RxRlcPDU));
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// adding for getting RSRP and SE value ///////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double CalculateSpectralEfficiency(double sinr) {
    // Shannon's formula to estimate spectral efficiency
    // This is a simplification and might not be accurate for all scenarios
    return std::log2(1 + sinr);
    //SINR to SE Calculation: The calculation of SE from SINR is a simplification using Shannon's formula. 
    //The actual SE calculation can be more complex and MCS-dependent in real-world scenarios.
}

void MySinrCallback(std::string path, uint16_t cellId, uint16_t rnti, double sinr, uint16_t bwpId, uint8_t ccId) {
    double spectralEfficiency = CalculateSpectralEfficiency(sinr);
    // std::cout << "Path: " << path << ", CellId: " << cellId << ", RNTI: " << rnti 
    //           << ", SINR: " << sinr << " dB, SE: " << spectralEfficiency << " bps/Hz" << std::endl;
    printf("Path: %s,\n CellId: %u,\n RNTI: %u,\n SINR: %lf dB,\n SE: %lf bps/Hz\n", 
       path.c_str(), cellId, rnti, sinr, spectralEfficiency);
}

// Callback for RSSI
void RssiCallback(double rssi)
{
    std::cout << "RSSI: " << rssi << " dBm" << std::endl;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// void CourseChangeCallback(Ptr<const MobilityModel> model) {
//     Vector position = model->GetPosition();
//     NS_LOG_UNCOND("Node at x: " << position.x << ", y: " << position.y);
// }

void LogPosition(NodeContainer nodes)
{
    for (NodeContainer::Iterator i = nodes.Begin(); i != nodes.End(); ++i)
    {
        Ptr<Node> node = (*i);
        Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
        Vector pos = mobility->GetPosition();
        std::cout << "Node " << node->GetId() << ": Position(" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
    // Schedule the next position log, make sure the time here is reasonable for your simulation
    // Simulator::Schedule(Seconds(1.0), &LogPosition, nodes);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float random_pri() {
    // Create a random number generator engine
    std::random_device rd;   // Use a hardware random device if available
    std::mt19937 gen(rd()); // Mersenne Twister engine for 32-bit integers

    // Define a uniform distribution for floating-point values between 0.0 and 1.0
    std::uniform_real_distribution<double> distribution(700.0, 800.0);

    // Generate and print random values with a step of 0.1
    double randomValue;
    for (double value = 700.0; value <= 800.0; value += 10.0) {
        randomValue = distribution(gen);
        // std::cout << "Random Value (" << value << "): " << randomValue << std::endl;
    }
    return randomValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Main Function //////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



int
main(int argc, char* argv[])
{

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Setting Packet size, Amount of UE and gNB, FrequencyBand ///////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    uint16_t numerologyBwp1 = 0;
    uint32_t udpPacketSize = 1000;
    double centralFrequencyBand1 = 28e9; // 28GHz
    double bandwidthBand1 = 400e6; // 400MHz
    uint16_t gNbNum = 1; // one gNodeB
    uint16_t ueNumPergNb = 1; // one UE device
    bool enableUl = false;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    Time sendPacketTime = Seconds(0.4);

    CommandLine cmd(__FILE__);
    cmd.AddValue("numerologyBwp1", "The numerology to be used in bandwidth part 1", numerologyBwp1);
    cmd.AddValue("centralFrequencyBand1",
                 "The system frequency to be used in band 1",
                 centralFrequencyBand1);
    cmd.AddValue("bandwidthBand1", "The system bandwidth to be used in band 1", bandwidthBand1);
    cmd.AddValue("packetSize", "packet size in bytes", udpPacketSize);
    cmd.AddValue("enableUl", "Enable Uplink", enableUl);
    cmd.Parse(argc, argv);

    int64_t randomStream = 1;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Create the scenario
    GridScenarioHelper gridScenario;
    gridScenario.SetRows(1);
    gridScenario.SetColumns(gNbNum);
    gridScenario.SetHorizontalBsDistance(5.0);
    gridScenario.SetBsHeight(10.0);
    gridScenario.SetUtHeight(1.5);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // must be set before BS number
    gridScenario.SetSectorization(GridScenarioHelper::SINGLE);
    gridScenario.SetBsNumber(gNbNum);
    gridScenario.SetUtNumber(ueNumPergNb * gNbNum);
    gridScenario.SetScenarioHeight(3); // Create a 3x3 scenario where the UE will
    gridScenario.SetScenarioLength(3); // be distribuited.
    randomStream += gridScenario.AssignStreams(randomStream);
    gridScenario.CreateScenario();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();

    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    nrHelper->SetEpcHelper(epcHelper);

    // Create one operational band containing one CC with one bandwidth part
    BandwidthPartInfoPtrVector allBwps;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Create the configuration for the CcBwpHelper
    CcBwpCreator::SimpleOperationBandConf bandConf1(centralFrequencyBand1,
                                                    bandwidthBand1,
                                                    numCcPerBand,
                                                    BandwidthPartInfo::UMi_StreetCanyon_LoS);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // By using the configuration created, it is time to make the operation band
    OperationBandInfo band1 = ccBwpCreator.CreateOperationBandContiguousCc(bandConf1);

    Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(0)));
    nrHelper->SetSchedulerAttribute("FixedMcsDl", BooleanValue(true));
    nrHelper->SetSchedulerAttribute("StartingMcsDl", UintegerValue(28));
    nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(0)));
    nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));

    nrHelper->InitializeOperationBand(&band1);
    allBwps = CcBwpCreator::GetAllBwps({band1});
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Beamforming method
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(DirectPathBeamforming::GetTypeId()));

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Antennas for all the UEs
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(2));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(4));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Antennas for all the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<IsotropicAntennaModel>()));
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Install and get the pointers to the NetDevices
    NetDeviceContainer enbNetDev =
        nrHelper->InstallGnbDevice(gridScenario.GetBaseStations(), allBwps);
    NetDeviceContainer ueNetDev =
        nrHelper->InstallUeDevice(gridScenario.GetUserTerminals(), allBwps);

    randomStream += nrHelper->AssignStreams(enbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Set the attribute of the netdevice (enbNetDev.Get (0)) and bandwidth part (0)
    nrHelper->GetGnbPhy(enbNetDev.Get(0), 0)
        ->SetAttribute("Numerology", UintegerValue(numerologyBwp1));

    for (auto it = enbNetDev.Begin(); it != enbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    for (auto it = ueNetDev.Begin(); it != ueNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    InternetStackHelper internet;
    internet.Install(gridScenario.GetUserTerminals());
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if (enableUl)
    {
        Simulator::Schedule(sendPacketTime,
                            &SendPacket,
                            ueNetDev.Get(0),
                            enbNetDev.Get(0)->GetAddress(),
                            udpPacketSize);
    }
    else
    {
        Simulator::Schedule(sendPacketTime,
                            &SendPacket,
                            enbNetDev.Get(0),
                            ueNetDev.Get(0)->GetAddress(),
                            udpPacketSize);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // attach UEs to the closest eNB
    nrHelper->AttachToClosestEnb(ueNetDev, enbNetDev);
    /////////////////////////////////////////////////////////////////////////////
// Define the maximum bounds for the mobility model
// double x_max = 50.0; // Maximum x-coordinate for UE movement
// double y_max = 50.0; // Maximum y-coordinate for UE movement
// double distance = 20.0; // Maximum distance a UE can cover in each random step
// double speed = 10.0; // Speed of the UE in meters/second

// Create a NodeContainer with the nodes from ueNetDev
NodeContainer ueNodes;
for (NetDeviceContainer::Iterator it = ueNetDev.Begin(); it != ueNetDev.End(); ++it) {
    ueNodes.Add((*it)->GetNode());
}

//  // Define a position allocator
//     Ptr<UniformRandomVariable> xVal = CreateObject<UniformRandomVariable>();
//     xVal->SetAttribute("Min", DoubleValue(0.0));
//     xVal->SetAttribute("Max", DoubleValue(100.0)); // adjust these values as needed

//     Ptr<UniformRandomVariable> yVal = CreateObject<UniformRandomVariable>();
//     yVal->SetAttribute("Min", DoubleValue(0.0));
//     yVal->SetAttribute("Max", DoubleValue(100.0)); // adjust these values as needed

//     Ptr<RandomRectanglePositionAllocator> positionAlloc = CreateObject<RandomRectanglePositionAllocator>();
//     positionAlloc->SetX(xVal);
//     positionAlloc->SetY(yVal);

//     // Setup Mobility Model
//     MobilityHelper mobility;
//     mobility.SetPositionAllocator(positionAlloc);
//     mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
//                               "Bounds", RectangleValue(Rectangle(-x_max, x_max, -y_max, y_max)),
//                               "Distance", DoubleValue(distance),
//                               "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"));
//     mobility.Install(ueNodes);
//     // Define UE nodes
//     NodeContainer ueNodes;
    // ueNodes.Create(ueNumPergNb * gNbNum); // Adjust ueNumPergNb and gNbNum as needed

    //Positioning UEs initially
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    // for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
        // positionAlloc->Add(Vector(5.0 * i, 5.0 * i, 0)); // Example positions, modify as needed
        double x = random_pri();
        double y = random_pri();
        positionAlloc->Add(Vector(x, y, 0)); 
    // }

    // Random Positioning UEs initially
// double minX = 0; // Maximum x-coordinate 
// double maxX = 50; // Maximum y-coordinate
// double minY = 0; // Maximum x-coordinate 
// double maxY = 50; // Maximum y-coordinate
// // Create random number generators for x and y coordinates
// Ptr<UniformRandomVariable> randomX = CreateObject<UniformRandomVariable>();
// randomX->SetAttribute("Min", DoubleValue(minX)); // Set the minimum value for x
// randomX->SetAttribute("Max", DoubleValue(maxX)); // Set the maximum value for x

// Ptr<UniformRandomVariable> randomY = CreateObject<UniformRandomVariable>();
// randomY->SetAttribute("Min", DoubleValue(minY)); // Set the minimum value for y
// randomY->SetAttribute("Max", DoubleValue(maxY)); // Set the maximum value for y

// // Positioning UEs initially
// Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
// for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
//     double x = randomX->GetValue();
//     double y = randomY->GetValue();
//     positionAlloc->Add(Vector(x, y, 0));
// }
/////  not finish /////////////////////////
    // // Set up the mobility model
    MobilityHelper mobility;
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)), // Modify bounds as needed
                              "Distance", DoubleValue(10.0), // Maximum distance a UE can cover in each random step
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5.0]")); // Modify speed as needed
    mobility.Install(ueNodes);


    for (uint32_t i = 0; i < ueNodes.GetN(); ++i) {
    Ptr<Node> node = ueNodes.Get(i);
    Vector pos = node->GetObject<MobilityModel>()->GetPosition();
    std::cout << "Initial position of Node " << node->GetId() << ": (" 
              << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
////////////////////////////////////////////////////////////////////////



// // Setup Mobility Model for UEs
// MobilityHelper mobility;
// mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
//                           "Bounds", RectangleValue(Rectangle(-x_max, x_max, -y_max, y_max)),
//                           "Distance", DoubleValue(distance),
//                           "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" + std::to_string(speed) + "]"));
// mobility.Install(ueNodes);


     /////////////////////////////////////////////////////////////////////////////
for (uint32_t i = 0; i < ueNetDev.GetN(); i++)
{
    Ptr<NrSpectrumPhy> ueSpectrumPhy = DynamicCast<NrUeNetDevice>(ueNetDev.Get(i))->GetPhy(0)->GetSpectrumPhy();
    Ptr<NrInterference> ueSpectrumPhyInterference = ueSpectrumPhy->GetNrInterference();
    NS_ABORT_IF(!ueSpectrumPhyInterference);
    ueSpectrumPhyInterference->TraceConnectWithoutContext("RssiPerProcessedChunk", MakeCallback(&RssiCallback));
}
 /////////////////////////////////////////////////////////////////////////////

    if (enableUl)
    {
        std::cout << "\n Sending data in uplink." << std::endl;
        Simulator::Schedule(Seconds(0.2), &ConnectUlPdcpRlcTraces);
    }
    else
    {
        std::cout << "\n Sending data in downlink." << std::endl;
        Simulator::Schedule(Seconds(0.2), &ConnectPdcpRlcTraces);
    }

    nrHelper->EnableTraces();

// Connect the SINR trace source to your callback
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/DlDataSinr", MakeCallback(&MySinrCallback));
    Simulator::Schedule(Seconds(0.5), &LogPosition, ueNodes); // Start logging positions after 1 second
    Simulator::Stop(Seconds(10));

    Simulator::Run();
    Simulator::Destroy();

    if (g_rxPdcpCallbackCalled && g_rxRxRlcPDUCallbackCalled)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}
