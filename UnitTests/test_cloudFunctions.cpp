#include "pch.h"
#include "CppUnitTest.h"
#include "../GCSDokan/googleAuth.h"
#include "../GCSDokan/cloudFunctions.h"
#include "string"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using std::string;
using std::wstring;

namespace GCSDokan
{
	TEST_CLASS(cloudFunctions)
	{
	public:
		TEST_METHOD(download_file)
		{
			auth_anonymous();
			string content("For more information on the data in this bucket see:\n");

			size_t offset = 4;
			size_t len = content.length() - offset;
			char buffer[50];
			wstring path = L"genomics-public-data/README";
			gcs_read_file(path, buffer, offset, len);
			for (int i = 0; i < len; i++) {
				Assert::AreEqual(buffer[i], content[i + offset]);
			}
		}

		TEST_METHOD(download_folder_meta)
		{
			auth_anonymous();
			wstring linux_path = L"genomics-public-data/clinvar";
			folder_meta_info meta = gcs_get_folder_meta(linux_path);
			Assert::AreEqual(meta.local_name.c_str(), L"clinvar");
			Assert::IsTrue(meta.file_meta_vector.find(L"README.txt")!= meta.file_meta_vector.end());
			if (meta.file_meta_vector.find(L"README.txt") != meta.file_meta_vector.end()) {
				Assert::AreEqual(
					meta.file_meta_vector.at(L"README.txt").remote_full_path.c_str(),
					L"genomics-public-data/clinvar/README.txt"
				);
			}
		}
	};
}
