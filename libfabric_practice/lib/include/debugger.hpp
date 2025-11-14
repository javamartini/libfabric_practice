/* Code courtesy of Riley Shipley. Thanks for the help Riley. :D */

#include <iostream>
#include <rdma/fabric.h>

class Debugger
{
public:
    void print_endpoint_address(char *endpoint_address, fid_av *address_vector)
    {
        char *endpoint_address_string = new char[128];
        size_t ep_string_buff_len = 128;
        std::cout << " has endpoint address " << fi_av_straddr(address_vector, endpoint_address, endpoint_address_string, &ep_string_buff_len);

        delete[] endpoint_address_string;
    }

    void print_capabilities(uint64_t capabilities)
    {
        std::cout << "Fabric capabilities: ";
        if (capabilities & FI_MSG)
            std::cout << "FI_MSG ";
        if (capabilities & FI_RMA)
            std::cout << "FI_RMA ";
        if (capabilities & FI_RMA_EVENT)
            std::cout << "FI_RMA_EVENT ";
        if (capabilities & FI_REMOTE_WRITE)
            std::cout << "FI_REMOTE_WRITE ";
        if (capabilities & FI_WRITE)
            std::cout << "FI_WRITE ";
        std::cout << std::endl;
    }

    void print_mode(uint64_t mode)
    {
        std::cout << "Mode: ";
        if (!mode)
            std::cout << "Not set" << std::endl;
        else
        {
            if (mode & FI_ASYNC_IOV)
                std::cout << "FI_ASYNC_IOV ";
            // if (mode & FI_BUFFERED_RECV)     std::cout << "FI_BUFFERED_RECV "; Available in newer versions of LibFabric
            if (mode & FI_CONTEXT)
                std::cout << "FI_CONTEXT ";
            if (mode & FI_CONTEXT2)
                std::cout << "FI_CONTEXT2 ";
            if (mode & FI_MSG_PREFIX)
                std::cout << "FI_MSG_PREFIX ";
            if (mode & FI_RX_CQ_DATA)
                std::cout << "FI_RX_CQ_DATA ";
            if (mode & FI_NOTIFY_FLAGS_ONLY)
                std::cout << "FI_NOTIFY_FLAGS_ONLY ";
            if (mode & FI_RESTRICTED_COMP)
                std::cout << "FI_RESTRICTED_COMP ";
            if (mode & FI_RX_CQ_DATA)
                std::cout << "FI_RX_CQ_DATA ";

            // This mode is deprecated but kept for backward compatability
            // if (mode & FI_LOCAL_MR)     std::cout << "FI_LOCAL_MR ";
            std::cout << std::endl;
        }
    }

    void print_address_format(ssize_t addr_format)
    {
        std::cout << "Address format: ";
        if (!addr_format)
            std::cout << "Not set";
        else if (addr_format == FI_ADDR_PSMX)
            std::cout << "FI_ADDR_PSMX";
        else if (addr_format == FI_ADDR_PSMX2)
            std::cout << "FI_ADDR_PSMX2";
        else if (addr_format == FI_FORMAT_UNSPEC)
            std::cout << "FI_FORMAT_UNSPEC";
        else if (addr_format == FI_SOCKADDR)
            std::cout << "FI_SOCKADDR";
        else if (addr_format == FI_SOCKADDR_IN)
            std::cout << "FI_SOCKADDR";
        else if (addr_format == FI_SOCKADDR_IB)
            std::cout << "FI_SOCKADDR";
        else
            std::cout << "Unknown";

        std::cout << std::endl;
    }

    void print_ep_type(fi_ep_type ep_type)
    {
        std::cout << "EP type: ";
        if (!ep_type)
            std::cout << "Not set";
        else if (ep_type == FI_EP_MSG)
            std::cout << "FI_EP_MSG";
        else if (ep_type == FI_EP_RDM)
            std::cout << "FI_EP_RDM";
        else if (ep_type == FI_EP_DGRAM)
            std::cout << "FI_EP_DGRAM";
        else
            std::cout << "Unknown";
        std::cout << std::endl;
    }

    void print_mr_mode(int mr_mode)
    {
        std::cout << "MR Mode: ";
        if (!mr_mode)
            std::cout << "Not set" << std::endl;
        else
        {
            if (mr_mode & FI_MR_ALLOCATED)
                std::cout << "FI_MR_ALLOCATED ";
            if (mr_mode & FI_MR_COLLECTIVE)
                std::cout << "FI_MR_COLLECTIVE ";
            if (mr_mode & FI_MR_ENDPOINT)
                std::cout << "FI_MR_ENDPOINT ";
            if (mr_mode & FI_MR_LOCAL)
                std::cout << "FI_CONTEXT ";
            if (mr_mode & FI_MR_MMU_NOTIFY)
                std::cout << "FI_MR_MMU_NOTIFY ";
            if (mr_mode & FI_MR_PROV_KEY)
                std::cout << "FI_MR_PROV_KEY ";
            if (mr_mode & FI_MR_RAW)
                std::cout << "FI_MR_RAW ";
            if (mr_mode & FI_MR_RMA_EVENT)
                std::cout << "FI_MR_RMA_EVENT ";
            if (mr_mode & FI_MR_VIRT_ADDR)
                std::cout << "FI_MR_VIRT_ADDR ";

            /*
             * These mr_modes are deprecated and will throw a warning,
             * however they can still be used for backward compatability
             * if (mr_mode & FI_MR_UNSPEC)     std::cout << "FI_MR_UNSPEC ";
             * if (mr_mode & FI_MR_BASIC)      std::cout << "FI_MR_BASIC ";
             * if (mr_mode & FI_MR_SCALABLE)   std::cout << "FI_MR_SCALABLE ";
             */
            std::cout << std::endl;
        }
    }

    void print_info(fi_info *info)
    {
        if (!info)
        {
            std::cout << "fi_info is NULL!" << std::endl;
            return;
        }

        print_capabilities(info->caps);

        std::cout << "Fabric Name: " << (info->fabric_attr ? info->fabric_attr->name : "NULL") << std::endl;
        std::cout << "Provider Name: " << (info->fabric_attr ? info->fabric_attr->prov_name : "NULL") << std::endl;
        std::cout << "Domain Name: " << (info->domain_attr ? info->domain_attr->name : "NULL") << std::endl;
        std::cout << "Fabric obj: " << (info->fabric_attr->fabric != nullptr ? info->fabric_attr->fabric : nullptr) << std::endl;
        std::cout << "Domain obj: " << (info->domain_attr->domain != nullptr ? info->domain_attr->domain : nullptr) << std::endl;

        print_mode(info->mode);
        print_address_format(info->addr_format);
        print_ep_type(info->ep_attr->type);

        if (!info->domain_attr)
            std::cout << "Domain attr not set" << std::endl;
        print_mr_mode(info->domain_attr->mr_mode);
    }
};
