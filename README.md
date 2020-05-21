# ICU (MS-ICU)

> See the official [International Components for Unicode](http://site.icu-project.org/) site for an introduction to ICU.

This repository is a fork of the [ICU](https://github.com/unicode-org/icu) project (specifically ICU4C) with some changes. This fork is maintained by the Microsoft Global Foundations team.

The changes in this repository fall into the following broad categories:
- Maintenance related changes.
- Changes that are required for usage internal to Microsoft.
- Changes that are needed for the Windows OS build of ICU.
- Additional locales from CLDR to improve parity with existing Windows NLS locale support.
- Changes to locale data for better compatibility with Windows, and to address issues from Windows customer feedback.

Before reporting any issues, or creating any pull-request(s), please ensure that your issue or change is related to one of the above reasons.

Any other issues, changes, bug fixes, improvements, enhancements, etc. should be made in the upstream project here:
- Upstream source: https://github.com/unicode-org/icu
- Upstream issues: https://unicode-org.atlassian.net/projects/ICU

## Contributing

> 👉 Note: Please make sure your change(s) cannot be made upstream first.

Any bug fixes, improvements, enhancements, etc. should be made in the upstream project here: https://github.com/unicode-org/icu

Otherwise, this project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.


## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

The contents of this repository are licensed under the terms described in the [LICENSE](LICENSE) file.
